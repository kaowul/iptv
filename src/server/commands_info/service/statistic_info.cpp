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

#include "server/commands_info/service/statistic_info.h"

#include <common/time.h>

#define STATISTIC_SERVICE_INFO_UPTIME_FIELD "uptime"
#define STATISTIC_SERVICE_INFO_TIMESTAMP_FIELD "timestamp"
#define STATISTIC_SERVICE_INFO_ID_FIELD "id"
#define STATISTIC_SERVICE_INFO_CPU_FIELD "cpu"
#define STATISTIC_SERVICE_INFO_GPU_FIELD "gpu"
#define STATISTIC_SERVICE_INFO_MEMORY_TOTAL_FIELD "memory_total"
#define STATISTIC_SERVICE_INFO_MEMORY_FREE_FIELD "memory_free"
#define STATISTIC_SERVICE_INFO_MEMORY_AVAILIBLE_FIELD "memory_availible"

#define STATISTIC_SERVICE_INFO_HDD_TOTAL_FIELD "hdd_total"
#define STATISTIC_SERVICE_INFO_HDD_FREE_FIELD "hdd_free"

#define STATISTIC_SERVICE_INFO_LOAD_AVERAGE_FIELD "load_average"

#define STATISTIC_SERVICE_INFO_BANDWIDTH_IN_FIELD "bandwidth_in"
#define STATISTIC_SERVICE_INFO_BANDWIDTH_OUT_FIELD "bandwidth_out"

#define STATISTIC_SERVICE_INFO_VERSION_FIELD "version"

namespace iptv_cloud {
namespace server {
namespace service {

StatisticServiceInfo::StatisticServiceInfo()
    : base_class(),
      node_id_(),
      cpu_load_(),
      gpu_load_(),
      uptime_(),
      mem_shot_(),
      hdd_shot_(),
      net_bytes_recv_(),
      net_bytes_send_(),
      current_ts_(),
      sys_shot_(),
      proj_ver_() {}

StatisticServiceInfo::StatisticServiceInfo(const std::string& node_id,
                                           int cpu_load,
                                           int gpu_load,
                                           const std::string& uptime,
                                           const utils::MemoryShot& mem_shot,
                                           const utils::HddShot& hdd_shot,
                                           uint64_t net_bytes_recv,
                                           uint64_t net_bytes_send,
                                           const utils::SysinfoShot& sys)
    : base_class(),
      node_id_(node_id),
      cpu_load_(cpu_load),
      gpu_load_(gpu_load),
      uptime_(uptime),
      mem_shot_(mem_shot),
      hdd_shot_(hdd_shot),
      net_bytes_recv_(net_bytes_recv),
      net_bytes_send_(net_bytes_send),
      current_ts_(common::time::current_mstime() / 1000),
      sys_shot_(sys),
      proj_ver_(PROJECT_VERSION_HUMAN) {}

common::Error StatisticServiceInfo::SerializeFields(json_object* out) const {
  json_object_object_add(out, STATISTIC_SERVICE_INFO_ID_FIELD, json_object_new_string(node_id_.c_str()));
  json_object_object_add(out, STATISTIC_SERVICE_INFO_CPU_FIELD, json_object_new_int(cpu_load_));
  json_object_object_add(out, STATISTIC_SERVICE_INFO_GPU_FIELD, json_object_new_int(gpu_load_));
  json_object_object_add(out, STATISTIC_SERVICE_INFO_LOAD_AVERAGE_FIELD, json_object_new_string(uptime_.c_str()));
  json_object_object_add(out, STATISTIC_SERVICE_INFO_MEMORY_TOTAL_FIELD, json_object_new_int64(mem_shot_.total_ram));
  json_object_object_add(out, STATISTIC_SERVICE_INFO_MEMORY_FREE_FIELD, json_object_new_int64(mem_shot_.free_ram));
  json_object_object_add(out, STATISTIC_SERVICE_INFO_MEMORY_AVAILIBLE_FIELD,
                         json_object_new_int64(mem_shot_.avail_ram));
  json_object_object_add(out, STATISTIC_SERVICE_INFO_HDD_TOTAL_FIELD, json_object_new_int64(hdd_shot_.hdd_total));
  json_object_object_add(out, STATISTIC_SERVICE_INFO_HDD_FREE_FIELD, json_object_new_int64(hdd_shot_.hdd_free));
  json_object_object_add(out, STATISTIC_SERVICE_INFO_BANDWIDTH_IN_FIELD, json_object_new_int64(net_bytes_recv_));
  json_object_object_add(out, STATISTIC_SERVICE_INFO_BANDWIDTH_OUT_FIELD, json_object_new_int64(net_bytes_send_));
  json_object_object_add(out, STATISTIC_SERVICE_INFO_UPTIME_FIELD, json_object_new_int64(sys_shot_.uptime));
  json_object_object_add(out, STATISTIC_SERVICE_INFO_TIMESTAMP_FIELD, json_object_new_int64(current_ts_));
  json_object_object_add(out, STATISTIC_SERVICE_INFO_VERSION_FIELD, json_object_new_string(proj_ver_.c_str()));
  return common::Error();
}

common::Error StatisticServiceInfo::DoDeSerialize(json_object* serialized) {
  StatisticServiceInfo inf;
  json_object* jnode_id = nullptr;
  json_bool jnode_id_exists = json_object_object_get_ex(serialized, STATISTIC_SERVICE_INFO_ID_FIELD, &jnode_id);
  if (!jnode_id_exists) {
    return common::make_error_inval();
  }
  inf.node_id_ = json_object_get_string(jnode_id);

  json_object* jcpu_load = nullptr;
  json_bool jcpu_load_exists = json_object_object_get_ex(serialized, STATISTIC_SERVICE_INFO_CPU_FIELD, &jcpu_load);
  if (jcpu_load_exists) {
    inf.cpu_load_ = json_object_get_int(jcpu_load);
  }

  json_object* jgpu_load = nullptr;
  json_bool jgpu_load_exists = json_object_object_get_ex(serialized, STATISTIC_SERVICE_INFO_GPU_FIELD, &jgpu_load);
  if (jgpu_load_exists) {
    inf.gpu_load_ = json_object_get_int(jgpu_load);
  }

  json_object* juptime = nullptr;
  json_bool juptime_exists = json_object_object_get_ex(serialized, STATISTIC_SERVICE_INFO_LOAD_AVERAGE_FIELD, &juptime);
  if (juptime_exists) {
    inf.uptime_ = json_object_get_string(juptime);
  }

  json_object* jmemory_total = nullptr;
  json_bool jmemory_total_exists =
      json_object_object_get_ex(serialized, STATISTIC_SERVICE_INFO_MEMORY_TOTAL_FIELD, &jmemory_total);
  if (jmemory_total_exists) {
    inf.mem_shot_.total_ram = json_object_get_int64(jmemory_total);
  }

  json_object* jmemory_free = nullptr;
  json_bool jmemory_free_exists =
      json_object_object_get_ex(serialized, STATISTIC_SERVICE_INFO_MEMORY_FREE_FIELD, &jmemory_free);
  if (jmemory_free_exists) {
    inf.mem_shot_.free_ram = json_object_get_int64(jmemory_free);
  }

  json_object* jmemory_avail = nullptr;
  json_bool jmemory_avail_exists =
      json_object_object_get_ex(serialized, STATISTIC_SERVICE_INFO_MEMORY_AVAILIBLE_FIELD, &jmemory_avail);
  if (jmemory_avail_exists) {
    inf.mem_shot_.avail_ram = json_object_get_int64(jmemory_avail);
  }

  json_object* jhdd_total = nullptr;
  json_bool jhdd_total_exists =
      json_object_object_get_ex(serialized, STATISTIC_SERVICE_INFO_HDD_TOTAL_FIELD, &jhdd_total);
  if (jhdd_total_exists) {
    inf.hdd_shot_.hdd_total = json_object_get_int64(jhdd_total);
  }

  json_object* jhdd_free = nullptr;
  json_bool jhdd_free_exists = json_object_object_get_ex(serialized, STATISTIC_SERVICE_INFO_HDD_FREE_FIELD, &jhdd_free);
  if (jhdd_free_exists) {
    inf.hdd_shot_.hdd_free = json_object_get_int64(jhdd_free);
  }

  json_object* jnet_bytes_recv = nullptr;
  json_bool jnet_bytes_recv_exists =
      json_object_object_get_ex(serialized, STATISTIC_SERVICE_INFO_BANDWIDTH_IN_FIELD, &jnet_bytes_recv);
  if (jnet_bytes_recv_exists) {
    inf.net_bytes_recv_ = json_object_get_int64(jnet_bytes_recv);
  }

  json_object* jnet_bytes_send = nullptr;
  json_bool jnet_bytes_send_exists =
      json_object_object_get_ex(serialized, STATISTIC_SERVICE_INFO_BANDWIDTH_OUT_FIELD, &jnet_bytes_send);
  if (jnet_bytes_send_exists) {
    inf.net_bytes_send_ = json_object_get_int64(jnet_bytes_send);
  }

  json_object* jsys_stamp = nullptr;
  json_bool jsys_stamp_exists = json_object_object_get_ex(serialized, STATISTIC_SERVICE_INFO_UPTIME_FIELD, &jsys_stamp);
  if (jsys_stamp_exists) {
    inf.sys_shot_.uptime = json_object_get_int64(jsys_stamp);
  }

  json_object* jcur_ts = nullptr;
  json_bool jcur_ts_exists = json_object_object_get_ex(serialized, STATISTIC_SERVICE_INFO_TIMESTAMP_FIELD, &jcur_ts);
  if (jcur_ts_exists) {
    inf.current_ts_ = json_object_get_int64(jcur_ts);
  }

  json_object* jproj_ver = nullptr;
  json_bool jproj_ver_exists = json_object_object_get_ex(serialized, STATISTIC_SERVICE_INFO_VERSION_FIELD, &jproj_ver);
  if (jproj_ver_exists) {
    inf.proj_ver_ = json_object_get_string(jproj_ver);
  }

  *this = inf;
  return common::Error();
}

std::string StatisticServiceInfo::GetNodeID() const {
  return node_id_;
}

int StatisticServiceInfo::GetCpuLoad() const {
  return cpu_load_;
}

int StatisticServiceInfo::GetGpuLoad() const {
  return gpu_load_;
}

std::string StatisticServiceInfo::GetUptime() const {
  return uptime_;
}

utils::MemoryShot StatisticServiceInfo::GetMemShot() const {
  return mem_shot_;
}

utils::HddShot StatisticServiceInfo::GetHddShot() const {
  return hdd_shot_;
}

uint64_t StatisticServiceInfo::GetNetBytesRecv() const {
  return net_bytes_recv_;
}

uint64_t StatisticServiceInfo::GetNetBytesSend() const {
  return net_bytes_send_;
}

time_t StatisticServiceInfo::GetTimestamp() const {
  return current_ts_;
}

std::string StatisticServiceInfo::GetProjectVersion() const {
  return proj_ver_;
}

}  // namespace service
}  // namespace server
}  // namespace iptv_cloud
