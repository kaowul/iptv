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

#pragma once

#include <string>

#include <common/libev/io_loop_observer.h>
#include <common/net/types.h>

#include "protocol/types.h"
#include "server/commands_info/start_stream_info.h"
#include "stats/istat.h"
#include "types.h"

namespace iptv_cloud {
namespace server {
class ChildStream;
namespace pipe {
class ProtocoledPipeClient;
}
class ProtocoledDaemonClient;

class ProcessSlaveWrapper : public common::libev::IoLoopObserver {
 public:
  enum { node_stats_send_seconds = 10, ping_timeout_clients_seconds = 60, cleanup_seconds = 5 };

  explicit ProcessSlaveWrapper(const std::string& licensy_key);
  ~ProcessSlaveWrapper() override;

  static int SendStopDaemonRequest(const std::string& license);

  int Exec(int argc, char** argv);

  std::string GetLogPath() const;

 protected:
  void PreLooped(common::libev::IoLoop* server) override;
  void Accepted(common::libev::IoClient* client) override;
  void Moved(common::libev::IoLoop* server,
             common::libev::IoClient* client) override;  // owner server, now client is orphan
  void Closed(common::libev::IoClient* client) override;
  void TimerEmited(common::libev::IoLoop* server, common::libev::timer_id_t id) override;

  void Accepted(common::libev::IoChild* child) override;
  void Moved(common::libev::IoLoop* server, common::libev::IoChild* child) override;
  void ChildStatusChanged(common::libev::IoChild* child, int status) override;

  void DataReceived(common::libev::IoClient* client) override;
  void DataReadyToWrite(common::libev::IoClient* client) override;
  void PostLooped(common::libev::IoLoop* server) override;

  virtual common::ErrnoError HandleRequestServiceCommand(ProtocoledDaemonClient* dclient,
                                                         protocol::request_t* req) WARN_UNUSED_RESULT;
  virtual common::ErrnoError HandleResponceServiceCommand(ProtocoledDaemonClient* dclient,
                                                          protocol::response_t* resp) WARN_UNUSED_RESULT;

  virtual common::ErrnoError HandleRequestStreamsCommand(pipe::ProtocoledPipeClient* pclient,
                                                         protocol::request_t* req) WARN_UNUSED_RESULT;
  virtual common::ErrnoError HandleResponceStreamsCommand(pipe::ProtocoledPipeClient* pclient,
                                                          protocol::response_t* resp) WARN_UNUSED_RESULT;

 private:
  ChildStream* FindChildByID(channel_id_t cid) const;
  void BroadcastClients(const protocol::request_t& req);

  struct NodeStats;
  common::ErrnoError DaemonDataReceived(ProtocoledDaemonClient* dclient) WARN_UNUSED_RESULT;
  common::ErrnoError PipeDataReceived(pipe::ProtocoledPipeClient* pclient) WARN_UNUSED_RESULT;

  protocol::sequance_id_t NextRequestID();

  common::ErrnoError CreateChildStream(common::libev::IoLoop* server, const StartStreamInfo& start_info);

  common::ErrnoError HandleRequestChangedSourcesStream(pipe::ProtocoledPipeClient* pclient,
                                                       protocol::request_t* req) WARN_UNUSED_RESULT;

  common::ErrnoError HandleRequestStatisticStream(pipe::ProtocoledPipeClient* pclient,
                                                  protocol::request_t* req) WARN_UNUSED_RESULT;

  common::ErrnoError HandleRequestClientStartStream(ProtocoledDaemonClient* dclient,
                                                    protocol::request_t* req) WARN_UNUSED_RESULT;
  common::ErrnoError HandleRequestClientStopStream(ProtocoledDaemonClient* dclient,
                                                   protocol::request_t* req) WARN_UNUSED_RESULT;
  common::ErrnoError HandleRequestClientRestartStream(ProtocoledDaemonClient* dclient,
                                                      protocol::request_t* req) WARN_UNUSED_RESULT;

  // service
  common::ErrnoError HandleRequestClientPrepareService(ProtocoledDaemonClient* dclient,
                                                       protocol::request_t* req) WARN_UNUSED_RESULT;
  common::ErrnoError HandleRequestClientActivate(ProtocoledDaemonClient* dclient,
                                                 protocol::request_t* req) WARN_UNUSED_RESULT;
  common::ErrnoError HandleRequestClientPingService(ProtocoledDaemonClient* dclient,
                                                    protocol::request_t* req) WARN_UNUSED_RESULT;
  common::ErrnoError HandleRequestClientStopService(ProtocoledDaemonClient* dclient,
                                                    protocol::request_t* req) WARN_UNUSED_RESULT;

  common::ErrnoError HandleResponcePingService(ProtocoledDaemonClient* dclient,
                                               protocol::response_t* resp) WARN_UNUSED_RESULT;

  void ReadConfig();
  void ClearStat();

  static common::net::HostAndPort GetServerHostAndPort();

  stats::IStat* stats_;  // all calls should be in loop thread

  std::string node_id_;
  const time_t start_time_;

  int process_argc_;
  char** process_argv_;

  std::string log_path_;

  common::libev::IoLoop* loop_;

  const std::string license_key_;
  std::atomic<protocol::seq_id_t> id_;
  common::libev::timer_id_t ping_client_id_timer_;
  common::libev::timer_id_t node_stats_timer_;
  common::libev::timer_id_t cleanup_timer_;
  NodeStats* node_stats_;
};

}  // namespace server
}  // namespace iptv_cloud
