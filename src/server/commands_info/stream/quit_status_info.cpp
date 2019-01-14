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

#include "server/commands_info/stream/quit_status_info.h"

#define STATUS_STREAM_INFO_STREAM_ID_FIELD "id"
#define STATUS_STREAM_INFO_SIGNAL_FIELD "signal"
#define STATUS_STREAM_INFO_EXIT_STATUS_FIELD "exit_status"

namespace iptv_cloud {
namespace server {
namespace stream {

QuitStatusInfo::QuitStatusInfo() : base_class(), stream_id_(), exit_status_(), signal_() {}

QuitStatusInfo::QuitStatusInfo(stream_id_t stream_id, int exit_status, int signal)
    : stream_id_(stream_id), exit_status_(exit_status), signal_(signal) {}

QuitStatusInfo::stream_id_t QuitStatusInfo::GetStreamID() const {
  return stream_id_;
}

int QuitStatusInfo::GetSignal() const {
  return signal_;
}

int QuitStatusInfo::GetExitStatus() const {
  return exit_status_;
}

common::Error QuitStatusInfo::DoDeSerialize(json_object* serialized) {
  QuitStatusInfo inf;
  json_object* jid = nullptr;
  json_bool jid_exists = json_object_object_get_ex(serialized, STATUS_STREAM_INFO_STREAM_ID_FIELD, &jid);
  if (!jid_exists) {
    return common::make_error_inval();
  }

  json_object* jsignal = nullptr;
  json_bool jsignal_exists = json_object_object_get_ex(serialized, STATUS_STREAM_INFO_SIGNAL_FIELD, &jsignal);
  if (!jsignal_exists) {
    return common::make_error_inval();
  }

  json_object* jexit_status = nullptr;
  json_bool jexit_status_exists =
      json_object_object_get_ex(serialized, STATUS_STREAM_INFO_EXIT_STATUS_FIELD, &jexit_status);
  if (!jexit_status_exists) {
    return common::make_error_inval();
  }

  inf.stream_id_ = json_object_get_string(jid);
  inf.signal_ = json_object_get_int(jsignal);
  inf.exit_status_ = json_object_get_int(jexit_status);
  *this = inf;
  return common::Error();
}

common::Error QuitStatusInfo::SerializeFields(json_object* out) const {
  json_object_object_add(out, STATUS_STREAM_INFO_STREAM_ID_FIELD, json_object_new_string(stream_id_.c_str()));
  json_object_object_add(out, STATUS_STREAM_INFO_SIGNAL_FIELD, json_object_new_int(signal_));
  json_object_object_add(out, STATUS_STREAM_INFO_EXIT_STATUS_FIELD, json_object_new_int(exit_status_));
  return common::Error();
}

}  // namespace stream
}  // namespace server
}  // namespace iptv_cloud
