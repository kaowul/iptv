/*  Copyright (C) 2014-2018 FastoGT. All right reserved.
    This file is part of iptv_cloud.
    iptv_cloud is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    iptv_cloud is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with iptv_cloud.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "server/process_slave_wrapper.h"

#include <sys/prctl.h>
#include <sys/wait.h>

#include <dlfcn.h>

#include <fstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <common/file_system/file_system.h>
#include <common/file_system/string_path_utils.h>
#include <common/net/net.h>
#include <common/string_util.h>

#include "child_stream.h"
#include "config_fields.h"
#include "inputs_outputs.h"
#include "stream_commands.h"

#include "stream/main_wrapper.h"

#include "pipe/pipe_client.h"

#include "server/commands_info/service/activate_info.h"
#include "server/commands_info/service/ping_info.h"
#include "server/commands_info/service/prepare_info.h"
#include "server/commands_info/service/statistic_info.h"
#include "server/commands_info/service/stop_info.h"
#include "server/commands_info/stream/quit_status_info.h"
#include "server/commands_info/stream/restart_info.h"
#include "server/commands_info/stream/stop_info.h"
#include "server/daemon_client.h"
#include "server/daemon_commands.h"
#include "server/daemon_server.h"
#include "server/options/options.h"
#include "server/stream_struct_utils.h"

#include "stream_commands_info/changed_sources_info.h"
#include "stream_commands_info/statistic_info.h"

#include "gpu_stats/perf_monitor.h"

#include "utils/arg_converter.h"
#include "utils/utils.h"

#define DUMMY_LOG_FILE_PATH "/dev/null"

#define DAEMON_STATS_1S "%s:stats"
#define SLAVE_STATS_1S "%s:slave"

#define CLIENT_PORT 6317

#define SERVICE_ID_FIELD "id"
#define SERVICE_STATS_CREDENTIALS_FIELD "stats_credentials"
#define SERVICE_LOG_FILE_FIELD "log_file"

namespace {

common::ErrnoError create_pipe(int* read_client_fd, int* write_client_fd) {
  if (!read_client_fd || !write_client_fd) {
    return common::make_errno_error_inval();
  }

  int pipefd[2] = {0};
  int res = pipe(pipefd);
  if (res == ERROR_RESULT_VALUE) {
    return common::make_errno_error(errno);
  }

  *read_client_fd = pipefd[0];
  *write_client_fd = pipefd[1];
  return common::ErrnoError();
}

std::string make_daemon_stats_id(std::string sid) {
  return common::MemSPrintf(DAEMON_STATS_1S, sid);
}

iptv_cloud::utils::ArgsMap read_slave_config(const std::string& path) {
  if (path.empty()) {
    CRITICAL_LOG() << "Invalid config path!";
  }

  std::ifstream config(path);
  if (!config.is_open()) {
    CRITICAL_LOG() << "Failed to open config path:" << path;
  }

  iptv_cloud::utils::ArgsMap options;
  std::string line;
  while (getline(config, line)) {
    const std::pair<std::string, std::string> pair = iptv_cloud::utils::GetKeyValue(line, '=');
    if (pair.first == SERVICE_ID_FIELD) {
      options.push_back(pair);
    } else if (pair.first == SERVICE_STATS_CREDENTIALS_FIELD) {
      options.push_back(pair);
    } else if (pair.first == SERVICE_LOG_FILE_FIELD) {
      options.push_back(pair);
    } else {
      WARNING_LOG() << "Unknown option: " << pair.first;
    }
  }

  return options;
}

}  // namespace

namespace iptv_cloud {
namespace {
std::string make_slave_stats_id(channel_id_t id) {
  return common::MemSPrintf(SLAVE_STATS_1S, id);
}

common::ErrnoError make_stream_info(const utils::ArgsMap& config_args, StreamInfo* sha) {
  if (!sha) {
    return common::make_errno_error_inval();
  }

  StreamInfo lsha;
  if (!utils::ArgsGetValue(config_args, ID_FIELD, &lsha.id)) {
    return common::make_errno_error("Define " ID_FIELD " variable and make it valid.", EAGAIN);
  }

  if (!utils::ArgsGetValue(config_args, TYPE_FIELD, &lsha.type)) {
    return common::make_errno_error("Define " TYPE_FIELD " variable and make it valid.", EAGAIN);
  }

  std::string feedback_dir;
  if (!utils::ArgsGetValue(config_args, FEEDBACK_DIR_FIELD, &feedback_dir)) {
    return common::make_errno_error("Define " FEEDBACK_DIR_FIELD " variable and make it valid.", EAGAIN);
  }

  if (!common::file_system::is_directory_exist(feedback_dir)) {
    common::ErrnoError errn = common::file_system::create_directory(feedback_dir, true);
    if (errn) {
      return errn;
    }
  }

  input_t input;
  if (!read_input(config_args, &input)) {
    return common::make_errno_error("Define " INPUT_FIELD " variable and make it valid.", EAGAIN);
  }

  for (auto input_uri : input) {
    lsha.input.push_back(input_uri.GetID());
  }

  bool is_multi_input = input.size() > 1;
  bool is_timeshift_and_rec =
      (lsha.type == TIMESHIFT_RECORDER && !is_multi_input) || (lsha.type == CATCHUP && !is_multi_input);
  if (is_timeshift_and_rec) {
    std::string timeshift_dir;
    if (utils::ArgsGetValue(config_args, TIMESHIFT_DIR_FIELD, &timeshift_dir)) {
      if (!common::file_system::is_directory_exist(timeshift_dir)) {
        common::ErrnoError errn = common::file_system::create_directory(timeshift_dir, true);
        if (errn) {
          return errn;
        }
      }
    }
    common::ErrnoError errn = common::file_system::node_access(timeshift_dir);
    if (errn) {
      return errn;
    }
  } else {
    output_t output;
    if (!read_output(config_args, &output)) {
      return common::make_errno_error("Define " OUTPUT_FIELD " variable and make it valid.", EAGAIN);
    }

    for (auto out_uri : output) {
      common::uri::Url ouri = out_uri.GetOutput();
      if (ouri.GetScheme() == common::uri::Url::http) {
        const common::file_system::ascii_directory_string_path http_root = out_uri.GetHttpRoot();
        const std::string http_root_str = http_root.GetPath();
        if (!common::file_system::is_directory_exist(http_root_str)) {
          common::ErrnoError errn = common::file_system::create_directory(http_root_str, true);
          if (errn) {
            return errn;
          }
        }
        common::ErrnoError errn = common::file_system::node_access(http_root_str);
        if (errn) {
          return errn;
        }
      }
      lsha.output.push_back(out_uri.GetID());
    }
  }

  *sha = lsha;
  return common::ErrnoError();
}
}  // namespace
namespace server {

struct ProcessSlaveWrapper::NodeStats {
  NodeStats() : prev(), prev_nshot(), gpu_load(0) {}

  utils::CpuShot prev;
  utils::NetShot prev_nshot;
  int gpu_load;
};

ProcessSlaveWrapper::ProcessSlaveWrapper(const std::string& license_key)
    : stats_(nullptr),
      node_id_(),
      start_time_(common::time::current_mstime() / 1000),
      loop_(),
      license_key_(license_key),
      id_(0),
      ping_client_id_timer_(INVALID_TIMER_ID),
      node_stats_timer_(INVALID_TIMER_ID),
      cleanup_timer_(INVALID_TIMER_ID),
      node_stats_(new NodeStats) {
  loop_ = new DaemonServer(GetServerHostAndPort(), this);
  loop_->SetName("back_end_server");
  ReadConfig();
}

int ProcessSlaveWrapper::SendStopDaemonRequest(const std::string& license) {
  if (license.empty()) {
    return EXIT_FAILURE;
  }

  service::StopInfo stop_req(license);
  std::string stop_str;
  common::Error serialize_error = stop_req.SerializeToString(&stop_str);
  if (serialize_error) {
    return EXIT_FAILURE;
  }

  protocol::request_t req = StopServiceRequest(protocol::MakeRequestID(0), stop_str);
  common::net::HostAndPort host = GetServerHostAndPort();
  common::net::socket_info client_info;
  common::ErrnoError err = common::net::connect(host, common::net::ST_SOCK_STREAM, 0, &client_info);
  if (err) {
    return EXIT_FAILURE;
  }

  std::unique_ptr<ProtocoledDaemonClient> connection(new ProtocoledDaemonClient(nullptr, client_info));
  err = connection->WriteRequest(req);
  if (err) {
    connection->Close();
    return EXIT_FAILURE;
  }

  connection->Close();
  return EXIT_SUCCESS;
}

ProcessSlaveWrapper::~ProcessSlaveWrapper() {
  ClearStat();
  destroy(&loop_);
  destroy(&node_stats_);
}

int ProcessSlaveWrapper::Exec(int argc, char** argv) {
  process_argc_ = argc;
  process_argv_ = argv;

  // gpu statistic monitor
  std::thread perf_thread;
  gpu_stats::IPerfMonitor* perf_monitor = gpu_stats::CreatePerfMonitor(&node_stats_->gpu_load);
  if (perf_monitor) {
    perf_thread = std::thread([perf_monitor] { perf_monitor->Exec(); });
  }

  int res = EXIT_FAILURE;
  common::libev::tcp::TcpServer* server = static_cast<common::libev::tcp::TcpServer*>(loop_);
  common::ErrnoError err = server->Bind(true);
  if (err) {
    DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    goto finished;
  }

  err = server->Listen(5);
  if (err) {
    DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    goto finished;
  }

  node_stats_->prev = utils::GetMachineCpuShot();
  node_stats_->prev_nshot = utils::GetMachineNetShot();
  res = server->Exec();

finished:
  if (perf_monitor) {
    perf_monitor->Stop();
  }
  if (perf_thread.joinable()) {
    perf_thread.join();
  }
  delete perf_monitor;
  return res;
}

void ProcessSlaveWrapper::ClearStat() {
  if (!stats_) {
    return;
  }

  destroy(&stats_);
}

void ProcessSlaveWrapper::PreLooped(common::libev::IoLoop* server) {
  ping_client_id_timer_ = server->CreateTimer(ping_timeout_clients_seconds, true);
  node_stats_timer_ = server->CreateTimer(node_stats_send_seconds, true);
}

void ProcessSlaveWrapper::Accepted(common::libev::IoClient* client) {
  UNUSED(client);  // DaemonClient
}

void ProcessSlaveWrapper::Moved(common::libev::IoLoop* server, common::libev::IoClient* client) {
  UNUSED(server);
  UNUSED(client);
}

void ProcessSlaveWrapper::Closed(common::libev::IoClient* client) {
  UNUSED(client);
}

void ProcessSlaveWrapper::TimerEmited(common::libev::IoLoop* server, common::libev::timer_id_t id) {
  if (ping_client_id_timer_ == id) {
    std::vector<common::libev::IoClient*> online_clients = server->GetClients();
    for (size_t i = 0; i < online_clients.size(); ++i) {
      common::libev::IoClient* client = online_clients[i];
      ProtocoledDaemonClient* dclient = dynamic_cast<ProtocoledDaemonClient*>(client);
      if (dclient && dclient->IsVerified()) {
        std::string ping_server_json;
        service::ServerPingInfo server_ping_info;
        common::Error err_ser = server_ping_info.SerializeToString(&ping_server_json);
        if (err_ser) {
          continue;
        }

        const protocol::request_t ping_request = PingDaemonRequest(NextRequestID(), ping_server_json);
        common::ErrnoError err = dclient->WriteRequest(ping_request);
        if (err) {
          DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
          err = client->Close();
          DCHECK(!err);
          delete client;
        } else {
          INFO_LOG() << "Pinged to client[" << client->GetFormatedName() << "], from server["
                     << server->GetFormatedName() << "], " << online_clients.size() << " client(s) connected.";
        }
      }
    }
  } else if (node_stats_timer_ == id) {
    if (stats_) {
      utils::CpuShot next = utils::GetMachineCpuShot();
      long double cpu_load = utils::GetCpuMachineLoad(node_stats_->prev, next);
      node_stats_->prev = next;

      utils::NetShot next_nshot = utils::GetMachineNetShot();
      uint64_t bytes_recv = (next_nshot.bytes_recv + node_stats_->prev_nshot.bytes_recv) / 2;
      uint64_t bytes_send = (next_nshot.bytes_send + node_stats_->prev_nshot.bytes_send) / 2;
      node_stats_->prev_nshot = next_nshot;

      utils::MemoryShot mem_shot = utils::GetMachineMemoryShot();
      utils::HddShot hdd_shot = utils::GetMachineHddShot();
      utils::SysinfoShot sshot = utils::GetMachineSysinfoShot();
      std::string uptime_str = common::MemSPrintf("%lu %lu %lu", sshot.loads[0], sshot.loads[1], sshot.loads[2]);

      service::StatisticInfo stat(node_id_, cpu_load * 100, node_stats_->gpu_load, uptime_str, mem_shot, hdd_shot,
                                  bytes_recv, bytes_send, sshot);
      std::string node_stats;
      common::Error err_ser = stat.SerializeToString(&node_stats);
      if (err_ser) {
        const std::string err_str = err_ser->GetDescription();
        WARNING_LOG() << "Failed to generate node statistic: " << err_str;
      }

      bool res = stats_->SetKey(make_daemon_stats_id(node_id_), node_stats);
      if (!res) {
        WARNING_LOG() << "Failed to save node statistic: " << node_stats;
      }

      BroadcastClients(StatisitcServiceBroadcast(node_stats));
    }
  } else if (cleanup_timer_ == id) {
    loop_->Stop();
  }
}

void ProcessSlaveWrapper::Accepted(common::libev::IoChild* child) {
  UNUSED(child);
}

void ProcessSlaveWrapper::Moved(common::libev::IoLoop* server, common::libev::IoChild* child) {
  UNUSED(server);
  UNUSED(child);
}

void ProcessSlaveWrapper::ChildStatusChanged(common::libev::IoChild* child, int status) {
  ChildStream* channel = static_cast<ChildStream*>(child);
  const channel_id_t cid = channel->GetChannelID();

  INFO_LOG() << "Successful finished children id: " << cid;
  int stabled_status = EXIT_SUCCESS;
  int signal_number = 0;

  if (WIFEXITED(status)) {
    stabled_status = WEXITSTATUS(status);
  } else {
    stabled_status = EXIT_FAILURE;
  }
  if (WIFSIGNALED(status)) {
    signal_number = WTERMSIG(status);
  }
  INFO_LOG() << "Stream id: " << cid << ", exit with status: " << (stabled_status ? "FAILURE" : "SUCCESS")
             << ", signal: " << signal_number;

  loop_->UnRegisterChild(child);
  delete child;

  std::string quit_json;
  stream::QuitStatusInfo ch_status_info(cid, stabled_status, signal_number);
  common::Error err_ser = ch_status_info.SerializeToString(&quit_json);
  if (err_ser) {
    const std::string err_str = err_ser->GetDescription();
    WARNING_LOG() << "Failed to generate strean exit message: " << err_str;
    return;
  }

  BroadcastClients(QuitStatusStreamBroadcast(quit_json));
}

ChildStream* ProcessSlaveWrapper::FindChildByID(channel_id_t cid) const {
  auto childs = loop_->GetChilds();
  for (auto* child : childs) {
    ChildStream* channel = static_cast<ChildStream*>(child);
    if (channel->GetChannelID() == cid) {
      return channel;
    }
  }

  return nullptr;
}

void ProcessSlaveWrapper::BroadcastClients(const protocol::request_t& req) {
  std::vector<common::libev::IoClient*> clients = loop_->GetClients();
  for (size_t i = 0; i < clients.size(); ++i) {
    ProtocoledDaemonClient* dclient = dynamic_cast<ProtocoledDaemonClient*>(clients[i]);
    if (dclient && dclient->IsVerified()) {
      dclient->WriteRequest(req);
    }
  }
}

common::ErrnoError ProcessSlaveWrapper::DaemonDataReceived(ProtocoledDaemonClient* dclient) {
  CHECK(loop_->IsLoopThread());
  std::string input_command;
  common::ErrnoError err = dclient->ReadCommand(&input_command);
  if (err) {
    return err;  // i don't want handle spam, comand must be foramated according
                 // protocol
  }

  protocol::request_t* req = nullptr;
  protocol::response_t* resp = nullptr;
  common::Error err_parse = common::protocols::json_rpc::ParseJsonRPC(input_command, &req, &resp);
  if (err_parse) {
    const std::string err_str = err_parse->GetDescription();
    return common::make_errno_error(err_str, EAGAIN);
  }

  if (req) {
    INFO_LOG() << "Received daemon request: " << input_command;
    err = HandleRequestServiceCommand(dclient, req);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    }
    delete req;
  } else if (resp) {
    INFO_LOG() << "Received daemon responce: " << input_command;
    err = HandleResponceServiceCommand(dclient, resp);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    }
    delete resp;
  } else {
    NOTREACHED();
    return common::make_errno_error("Invalid command type.", EINVAL);
  }

  return common::ErrnoError();
}

common::ErrnoError ProcessSlaveWrapper::PipeDataReceived(pipe::ProtocoledPipeClient* pipe_client) {
  CHECK(loop_->IsLoopThread());
  std::string input_command;
  common::ErrnoError err = pipe_client->ReadCommand(&input_command);
  if (err) {
    return err;  // i don't want handle spam, command must be foramated according
                 // protocol
  }

  protocol::request_t* req = nullptr;
  protocol::response_t* resp = nullptr;
  common::Error err_parse = common::protocols::json_rpc::ParseJsonRPC(input_command, &req, &resp);
  if (err_parse) {
    const std::string err_str = err_parse->GetDescription();
    return common::make_errno_error(err_str, EAGAIN);
  }

  if (req) {
    INFO_LOG() << "Received stream responce: " << input_command;
    err = HandleRequestStreamsCommand(pipe_client, req);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    }
    delete req;
  } else if (resp) {
    INFO_LOG() << "Received stream responce: " << input_command;
    err = HandleResponceStreamsCommand(pipe_client, resp);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    }
    delete resp;
  } else {
    NOTREACHED();
    return common::make_errno_error("Invalid command type.", EINVAL);
  }
  return common::ErrnoError();
}

void ProcessSlaveWrapper::DataReceived(common::libev::IoClient* client) {
  if (ProtocoledDaemonClient* dclient = dynamic_cast<ProtocoledDaemonClient*>(client)) {
    common::ErrnoError err = DaemonDataReceived(dclient);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      dclient->Close();
      delete dclient;
    }
  } else if (pipe::ProtocoledPipeClient* pipe_client = dynamic_cast<pipe::ProtocoledPipeClient*>(client)) {
    common::ErrnoError err = PipeDataReceived(pipe_client);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      auto childs = loop_->GetChilds();
      for (auto* child : childs) {
        ChildStream* channel = static_cast<ChildStream*>(child);
        if (pipe_client == channel->GetPipe()) {
          channel->SetPipe(nullptr);
          break;
        }
      }

      pipe_client->Close();
      delete pipe_client;
    }
  } else {
    NOTREACHED();
  }
}

void ProcessSlaveWrapper::DataReadyToWrite(common::libev::IoClient* client) {
  UNUSED(client);  // DaemonClient
}

void ProcessSlaveWrapper::PostLooped(common::libev::IoLoop* server) {
  if (ping_client_id_timer_ != INVALID_TIMER_ID) {
    server->RemoveTimer(ping_client_id_timer_);
    ping_client_id_timer_ = INVALID_TIMER_ID;
  }

  if (node_stats_timer_ != INVALID_TIMER_ID) {
    server->RemoveTimer(node_stats_timer_);
    node_stats_timer_ = INVALID_TIMER_ID;
  }
}

common::ErrnoError ProcessSlaveWrapper::HandleRequestClientStopService(ProtocoledDaemonClient* dclient,
                                                                       protocol::request_t* req) {
  CHECK(loop_->IsLoopThread());
  if (req->params) {
    const char* params_ptr = req->params->c_str();
    json_object* jstop = json_tokener_parse(params_ptr);
    if (!jstop) {
      return common::make_errno_error_inval();
    }

    service::StopInfo stop_info;
    common::Error err_des = stop_info.DeSerialize(jstop);
    json_object_put(jstop);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    bool is_verified_request = stop_info.GetLicense() == license_key_ || dclient->IsVerified();
    if (!is_verified_request) {
      return common::make_errno_error_inval();
    }

    if (cleanup_timer_ != INVALID_TIMER_ID) {
      // in progress
      protocol::response_t resp = StopServiceResponceFail(req->id, "Stop service in progress...");
      dclient->WriteResponce(resp);

      return common::ErrnoError();
    }

    auto childs = loop_->GetChilds();
    for (auto* child : childs) {
      ChildStream* channel = static_cast<ChildStream*>(child);
      channel->SendStop(NextRequestID());
    }

    protocol::response_t resp = StopServiceResponceSuccess(req->id);
    dclient->WriteResponce(resp);

    cleanup_timer_ = loop_->CreateTimer(cleanup_seconds, false);
    return common::ErrnoError();
  }

  return common::make_errno_error_inval();
}

common::ErrnoError ProcessSlaveWrapper::HandleResponcePingService(ProtocoledDaemonClient* dclient,
                                                                  protocol::response_t* resp) {
  UNUSED(dclient);
  CHECK(loop_->IsLoopThread());
  if (resp->IsMessage()) {
    const char* params_ptr = resp->message->result.c_str();
    json_object* jclient_ping = json_tokener_parse(params_ptr);
    if (!jclient_ping) {
      return common::make_errno_error_inval();
    }

    service::ClientPingInfo client_ping_info;
    common::Error err_des = client_ping_info.DeSerialize(jclient_ping);
    json_object_put(jclient_ping);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }
    return common::ErrnoError();
  }
  return common::ErrnoError();
}

common::ErrnoError ProcessSlaveWrapper::CreateChildStream(common::libev::IoLoop* server,
                                                          const stream::StartInfo& start_info) {
  CHECK(loop_->IsLoopThread());
  const std::string config_str = start_info.GetConfig();

  utils::ArgsMap config_args = options::ValidateConfig(config_str);
  StreamInfo sha;
  common::ErrnoError err = make_stream_info(config_args, &sha);
  if (err) {
    return err;
  }

  ChildStream* stream = FindChildByID(sha.id);
  if (stream) {
    NOTICE_LOG() << "Skip request to start stream id: " << sha.id;
    return common::make_errno_error(common::MemSPrintf("Stream with id: %s exist, skip request.", sha.id), EINVAL);
  }

  StreamStruct* mem = nullptr;
  err = AllocSharedStreamStruct(sha, &mem);
  if (err) {
    return err;
  }

  int read_command_client = 0;
  int write_requests_client = 0;
  err = create_pipe(&read_command_client, &write_requests_client);
  if (err) {
    FreeSharedStreamStruct(&mem);
    return err;
  }

  int read_responce_client = 0;
  int write_responce_client = 0;
  err = create_pipe(&read_responce_client, &write_responce_client);
  if (err) {
    FreeSharedStreamStruct(&mem);
    return err;
  }

#if !defined(TEST)
  pid_t pid = fork();
#else
  pid_t pid = 0;
#endif
  if (pid == 0) {  // child
    typedef int (*stream_exec_t)(const char* process_name, const struct cmd_args* cmd_args, void* config_args,
                                 void* command_client, void* mem);
    const std::string absolute_source_dir = common::file_system::absolute_path_from_relative(RELATIVE_SOURCE_DIR);
    const std::string lib_full_path = common::file_system::make_path(absolute_source_dir, CORE_LIBRARY);
    void* handle = dlopen(lib_full_path.c_str(), RTLD_LAZY);
    if (!handle) {
      ERROR_LOG() << "Failed to load " CORE_LIBRARY " path: " << lib_full_path
                  << ", error: " << common::common_strerror(errno);
      _exit(EXIT_FAILURE);
    }

    stream_exec_t stream_exec_func = reinterpret_cast<stream_exec_t>(dlsym(handle, "stream_exec"));
    char* error = dlerror();
    if (error) {
      ERROR_LOG() << "Failed to load start stream function error: " << error;
      dlclose(handle);
      _exit(EXIT_FAILURE);
    }

    std::string feedback_dir;
    if (!utils::ArgsGetValue(config_args, FEEDBACK_DIR_FIELD, &feedback_dir)) {
      ERROR_LOG() << "Define " FEEDBACK_DIR_FIELD " variable and make it valid.";
      dlclose(handle);
      _exit(EXIT_FAILURE);
    }

    int logs_level;
    if (!utils::ArgsGetValue(config_args, LOG_LEVEL_FIELD, &logs_level)) {
      logs_level = common::logging::LOG_LEVEL_DEBUG;
    }

    const struct cmd_args client_args = {feedback_dir.c_str(), logs_level};
    const std::string new_process_name = common::MemSPrintf(STREAMER_NAME " %s", sha.id);
    for (int i = 0; i < process_argc_; ++i) {
      memset(process_argv_[i], 0, strlen(process_argv_[i]));
    }
    const char* new_name = new_process_name.c_str();
    char* app_name = process_argv_[0];
    strncpy(app_name, new_name, new_process_name.length());
    app_name[new_process_name.length()] = 0;
    prctl(PR_SET_NAME, new_name);

#if !defined(TEST)
    // close not needed pipes
    common::ErrnoError errn = common::file_system::close_descriptor(read_responce_client);
    if (errn) {
      DEBUG_MSG_ERROR(errn, common::logging::LOG_LEVEL_WARNING);
    }
    errn = common::file_system::close_descriptor(write_requests_client);
    if (errn) {
      DEBUG_MSG_ERROR(errn, common::logging::LOG_LEVEL_WARNING);
    }
#endif

    pipe::ProtocoledPipeClient* client =
        new pipe::ProtocoledPipeClient(nullptr, read_command_client, write_responce_client);
    int res = stream_exec_func(new_name, &client_args, &config_args, client, mem);
    dlclose(handle);
    client->Close();
    delete client;
    _exit(res);
  } else if (pid < 0) {
    NOTICE_LOG() << "Failed to start children!";
  } else {
    // close not needed pipes
    common::ErrnoError errn = common::file_system::close_descriptor(read_command_client);
    if (errn) {
      DEBUG_MSG_ERROR(errn, common::logging::LOG_LEVEL_WARNING);
    }
    errn = common::file_system::close_descriptor(write_responce_client);
    if (err) {
      DEBUG_MSG_ERROR(errn, common::logging::LOG_LEVEL_WARNING);
    }

    pipe::ProtocoledPipeClient* pipe_client =
        new pipe::ProtocoledPipeClient(server, read_responce_client, write_requests_client);
    server->RegisterClient(pipe_client);
    ChildStream* new_channel = new ChildStream(loop_, mem);
    new_channel->SetPipe(pipe_client);
    loop_->RegisterChild(new_channel, pid);
  }

  return common::ErrnoError();
}

common::ErrnoError ProcessSlaveWrapper::HandleRequestChangedSourcesStream(pipe::ProtocoledPipeClient* pclient,
                                                                          protocol::request_t* req) {
  UNUSED(pclient);
  CHECK(loop_->IsLoopThread());
  if (req->params) {
    const char* params_ptr = req->params->c_str();
    json_object* jrequest_changed_sources = json_tokener_parse(params_ptr);
    if (!jrequest_changed_sources) {
      return common::make_errno_error_inval();
    }

    ChangedSouresInfo ch_sources_info;
    common::Error err_des = ch_sources_info.DeSerialize(jrequest_changed_sources);
    json_object_put(jrequest_changed_sources);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    std::string changed_json;
    common::Error err_ser = ch_sources_info.SerializeToString(&changed_json);
    if (err_ser) {
      const std::string err_str = err_ser->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    BroadcastClients(ChangedSourcesStreamBroadcast(changed_json));
    return common::ErrnoError();
  }

  return common::make_errno_error_inval();
}

common::ErrnoError ProcessSlaveWrapper::HandleRequestStatisticStream(pipe::ProtocoledPipeClient* pclient,
                                                                     protocol::request_t* req) {
  UNUSED(pclient);
  CHECK(loop_->IsLoopThread());
  if (req->params) {
    const char* params_ptr = req->params->c_str();
    json_object* jrequest_stat = json_tokener_parse(params_ptr);
    if (!jrequest_stat) {
      return common::make_errno_error_inval();
    }

    StatisticInfo stat;
    common::Error err_des = stat.DeSerialize(jrequest_stat);
    json_object_put(jrequest_stat);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    std::string stream_stats;
    common::Error err_ser = stat.SerializeToString(&stream_stats);
    if (err_ser) {
      const std::string err_str = err_ser->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    auto struc = stat.GetStreamStruct();
    bool res = stats_->SetKey(make_slave_stats_id(struc->id), stream_stats);
    if (!res) {
      WARNING_LOG() << "Failed to save stream statistic: " << stream_stats;
    }

    BroadcastClients(StatisitcStreamBroadcast(stream_stats));
    return common::ErrnoError();
  }

  return common::make_errno_error_inval();
}

common::ErrnoError ProcessSlaveWrapper::HandleRequestClientStartStream(ProtocoledDaemonClient* dclient,
                                                                       protocol::request_t* req) {
  CHECK(loop_->IsLoopThread());
  if (!dclient->IsVerified()) {
    return common::make_errno_error_inval();
  }

  if (req->params) {
    const char* params_ptr = req->params->c_str();
    json_object* jstart_info = json_tokener_parse(params_ptr);
    if (!jstart_info) {
      return common::make_errno_error_inval();
    }

    stream::StartInfo start_info;
    common::Error err_des = start_info.DeSerialize(jstart_info);
    json_object_put(jstart_info);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    common::libev::IoLoop* server = dclient->GetServer();
    common::ErrnoError err = CreateChildStream(server, start_info);
    if (err) {
      protocol::response_t resp = StartStreamResponceFail(req->id, err->GetDescription());
      dclient->WriteResponce(resp);
      return err;
    }

    protocol::response_t resp = StartStreamResponceSuccess(req->id);
    dclient->WriteResponce(resp);
    return common::ErrnoError();
  }

  return common::make_errno_error_inval();
}

protocol::sequance_id_t ProcessSlaveWrapper::NextRequestID() {
  const protocol::seq_id_t next_id = id_++;
  return protocol::MakeRequestID(next_id);
}

common::ErrnoError ProcessSlaveWrapper::HandleRequestClientStopStream(ProtocoledDaemonClient* dclient,
                                                                      protocol::request_t* req) {
  CHECK(loop_->IsLoopThread());
  if (!dclient->IsVerified()) {
    return common::make_errno_error_inval();
  }

  if (req->params) {
    const char* params_ptr = req->params->c_str();
    json_object* jstop_info = json_tokener_parse(params_ptr);
    if (!jstop_info) {
      return common::make_errno_error_inval();
    }

    stream::StopInfo stop_info;
    common::Error err_des = stop_info.DeSerialize(jstop_info);
    json_object_put(jstop_info);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    ChildStream* chan = FindChildByID(stop_info.GetStreamID());
    if (!chan) {
      protocol::response_t resp = StopStreamResponceFail(req->id, "Stream not found.");
      dclient->WriteResponce(resp);
      return common::ErrnoError();
    }

    chan->SendStop(NextRequestID());
    protocol::response_t resp = StopStreamResponceSuccess(req->id);
    dclient->WriteResponce(resp);
    return common::ErrnoError();
  }

  return common::make_errno_error_inval();
}

common::ErrnoError ProcessSlaveWrapper::HandleRequestClientRestartStream(ProtocoledDaemonClient* dclient,
                                                                         protocol::request_t* req) {
  CHECK(loop_->IsLoopThread());
  if (!dclient->IsVerified()) {
    return common::make_errno_error_inval();
  }

  if (req->params) {
    const char* params_ptr = req->params->c_str();
    json_object* jrestart_info = json_tokener_parse(params_ptr);
    if (!jrestart_info) {
      return common::make_errno_error_inval();
    }

    stream::RestartInfo restart_info;
    common::Error err_des = restart_info.DeSerialize(jrestart_info);
    json_object_put(jrestart_info);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    ChildStream* chan = FindChildByID(restart_info.GetStreamID());
    if (!chan) {
      protocol::response_t resp = RestartStreamResponceFail(req->id, "Stream not found.");
      dclient->WriteResponce(resp);
      return common::ErrnoError();
    }

    chan->SendRestart(NextRequestID());
    protocol::response_t resp = RestartStreamResponceSuccess(req->id);
    dclient->WriteResponce(resp);
    return common::ErrnoError();
  }

  return common::make_errno_error_inval();
}

common::ErrnoError ProcessSlaveWrapper::HandleRequestClientPrepareService(ProtocoledDaemonClient* dclient,
                                                                          protocol::request_t* req) {
  CHECK(loop_->IsLoopThread());
  if (!dclient->IsVerified()) {
    return common::make_errno_error_inval();
  }

  if (req->params) {
    const char* params_ptr = req->params->c_str();
    json_object* jservice_state = json_tokener_parse(params_ptr);
    if (!jservice_state) {
      return common::make_errno_error_inval();
    }

    service::PrepareInfo state_info;
    common::Error err_des = state_info.DeSerialize(jservice_state);
    json_object_put(jservice_state);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    service::Directories dirs(state_info);
    std::string resp_str = service::MakeDirectoryResponce(dirs);
    protocol::response_t resp = StateServiceResponce(req->id, resp_str);
    dclient->WriteResponce(resp);
    return common::ErrnoError();
  }

  return common::make_errno_error_inval();
}

common::ErrnoError ProcessSlaveWrapper::HandleRequestClientActivate(ProtocoledDaemonClient* dclient,
                                                                    protocol::request_t* req) {
  CHECK(loop_->IsLoopThread());
  if (req->params) {
    const char* params_ptr = req->params->c_str();
    json_object* jactivate = json_tokener_parse(params_ptr);
    if (!jactivate) {
      return common::make_errno_error_inval();
    }

    service::ActivateInfo activate_info;
    common::Error err_des = activate_info.DeSerialize(jactivate);
    json_object_put(jactivate);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      protocol::response_t resp = ActivateResponceFail(req->id, err_str);
      dclient->WriteResponce(resp);
      return common::make_errno_error(err_str, EAGAIN);
    }

    bool is_active = activate_info.GetLicense() == license_key_;
    if (!is_active) {
      protocol::response_t resp = ActivateResponceFail(req->id, "Wrong license key");
      dclient->WriteResponce(resp);
      return common::make_errno_error_inval();
    }

    protocol::response_t resp = ActivateResponceSuccess(req->id);
    dclient->WriteResponce(resp);
    dclient->SetVerified(true);
    return common::ErrnoError();
  }

  return common::make_errno_error_inval();
}

common::ErrnoError ProcessSlaveWrapper::HandleRequestClientPingService(ProtocoledDaemonClient* dclient,
                                                                       protocol::request_t* req) {
  CHECK(loop_->IsLoopThread());
  if (!dclient->IsVerified()) {
    return common::make_errno_error_inval();
  }

  if (req->params) {
    const char* params_ptr = req->params->c_str();
    json_object* jstop = json_tokener_parse(params_ptr);
    if (!jstop) {
      return common::make_errno_error_inval();
    }

    service::ClientPingInfo client_ping_info;
    common::Error err_des = client_ping_info.DeSerialize(jstop);
    json_object_put(jstop);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    std::string ping_server_json;
    service::ServerPingInfo server_ping_info;
    common::Error err_ser = server_ping_info.SerializeToString(&ping_server_json);
    if (err_ser) {
      const std::string err_str = err_ser->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    protocol::response_t resp = PingServiceResponce(req->id, ping_server_json);
    dclient->WriteResponce(resp);
    return common::ErrnoError();
  }

  return common::make_errno_error_inval();
}

common::ErrnoError ProcessSlaveWrapper::HandleRequestServiceCommand(ProtocoledDaemonClient* dclient,
                                                                    protocol::request_t* req) {
  if (req->method == CLIENT_START_STREAM) {
    return HandleRequestClientStartStream(dclient, req);
  } else if (req->method == CLIENT_STOP_STREAM) {
    return HandleRequestClientStopStream(dclient, req);
  } else if (req->method == CLIENT_RESTART_STREAM) {
    return HandleRequestClientRestartStream(dclient, req);
  } else if (req->method == CLIENT_PREPARE_SERVICE) {
    return HandleRequestClientPrepareService(dclient, req);
  } else if (req->method == CLIENT_STOP_SERVICE) {
    return HandleRequestClientStopService(dclient, req);
  } else if (req->method == CLIENT_ACTIVATE) {
    return HandleRequestClientActivate(dclient, req);
  } else if (req->method == CLIENT_PING_SERVICE) {
    return HandleRequestClientPingService(dclient, req);
  }

  WARNING_LOG() << "Received unknown method: " << req->method;
  return common::ErrnoError();
}

common::ErrnoError ProcessSlaveWrapper::HandleResponceServiceCommand(ProtocoledDaemonClient* dclient,
                                                                     protocol::response_t* resp) {
  CHECK(loop_->IsLoopThread());
  if (!dclient->IsVerified()) {
    return common::make_errno_error_inval();
  }

  protocol::request_t req;
  if (dclient->PopRequestByID(resp->id, &req)) {
    if (req.method == SERVER_PING) {
      HandleResponcePingService(dclient, resp);
    }
  }

  return common::ErrnoError();
}

common::ErrnoError ProcessSlaveWrapper::HandleRequestStreamsCommand(pipe::ProtocoledPipeClient* pclient,
                                                                    protocol::request_t* req) {
  if (req->method == CHANGED_SOURCES_STREAM) {
    return HandleRequestChangedSourcesStream(pclient, req);
  } else if (req->method == STATISTIC_STREAM) {
    return HandleRequestStatisticStream(pclient, req);
  }

  WARNING_LOG() << "Received unknown command: " << req->method;
  return common::ErrnoError();
}

common::ErrnoError ProcessSlaveWrapper::HandleResponceStreamsCommand(pipe::ProtocoledPipeClient* pclient,
                                                                     protocol::response_t* resp) {
  protocol::request_t req;
  if (pclient->PopRequestByID(resp->id, &req)) {
    if (req.method == STOP_STREAM) {
    } else if (req.method == RESTART_STREAM) {
    }
  }
  return common::ErrnoError();
}

void ProcessSlaveWrapper::ReadConfig() {  // CONFIG_SLAVE_FILE_PATH
  utils::ArgsMap slave_config_args = read_slave_config(CONFIG_SLAVE_FILE_PATH);
  if (!utils::ArgsGetValue(slave_config_args, SERVICE_ID_FIELD, &node_id_)) {
    CRITICAL_LOG() << "Define " SERVICE_ID_FIELD " variable and make it valid.";
  }

  std::string stats_data;
  if (!utils::ArgsGetValue(slave_config_args, SERVICE_STATS_CREDENTIALS_FIELD, &stats_data)) {
    CRITICAL_LOG() << "Define " SERVICE_STATS_CREDENTIALS_FIELD " variable and make it valid.";
  }

  if (!utils::ArgsGetValue(slave_config_args, SERVICE_LOG_FILE_FIELD, &log_path_)) {
    WARNING_LOG() << "Define " SERVICE_LOG_FILE_FIELD " variable and make it valid, now used: " DUMMY_LOG_FILE_PATH ".";
    log_path_ = DUMMY_LOG_FILE_PATH;
  }

  stats::StatCredentialsBase* screds = stats::StatCredentialsBase::CreateCreadentialsFromString(stats_data);
  CHECK(screds);
  ClearStat();
  stats_ = stats::IStat::CreateStat(screds);
}

common::net::HostAndPort ProcessSlaveWrapper::GetServerHostAndPort() {
  return common::net::HostAndPort::CreateLocalHost(CLIENT_PORT);
}

std::string ProcessSlaveWrapper::GetLogPath() const {
  return log_path_;
}

}  // namespace server
}  // namespace iptv_cloud
