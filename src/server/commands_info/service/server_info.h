/*  Copyright (C) 2014-2019 FastoGT. All right reserved.
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

#include <common/net/types.h>
#include <common/serializer/json_serializer.h>

#include "utils/utils.h"

namespace iptv_cloud {
namespace server {
namespace service {

class ServerInfo : public common::serializer::JsonSerializer<ServerInfo> {
 public:
  typedef JsonSerializer<ServerInfo> base_class;
  ServerInfo();
  explicit ServerInfo(int cpu_load,
                      int gpu_load,
                      const std::string& uptime,
                      const utils::MemoryShot& mem_shot,
                      const utils::HddShot& hdd_shot,
                      uint64_t net_bytes_recv,
                      uint64_t net_bytes_send,
                      const utils::SysinfoShot& sys,
                      time_t timestamp);

  int GetCpuLoad() const;
  int GetGpuLoad() const;
  std::string GetUptime() const;
  utils::MemoryShot GetMemShot() const;
  utils::HddShot GetHddShot() const;
  uint64_t GetNetBytesRecv() const;
  uint64_t GetNetBytesSend() const;
  time_t GetTimestamp() const;

 protected:
  common::Error DoDeSerialize(json_object* serialized) override;
  common::Error SerializeFields(json_object* out) const override;

 private:
  int cpu_load_;
  int gpu_load_;
  std::string uptime_;
  utils::MemoryShot mem_shot_;
  utils::HddShot hdd_shot_;
  uint64_t net_bytes_recv_;
  uint64_t net_bytes_send_;
  time_t current_ts_;
  utils::SysinfoShot sys_shot_;
};

class FullServiceInfo : public ServerInfo {
 public:
  typedef ServerInfo base_class;
  FullServiceInfo();
  explicit FullServiceInfo(const std::string& node_id,
                           const common::net::HostAndPort& http_host,
                           const base_class& base);

  std::string GetNodeID() const;
  common::net::HostAndPort GetHttpHost() const;
  std::string GetProjectVersion() const;

 protected:
  common::Error DoDeSerialize(json_object* serialized) override;
  common::Error SerializeFields(json_object* out) const override;

 private:
  std::string node_id_;
  common::net::HostAndPort http_host_;
  std::string proj_ver_;
};

}  // namespace service
}  // namespace server
}  // namespace iptv_cloud
