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

#include "protocol/types.h"

namespace iptv_cloud {
namespace protocol {

common::protocols::json_rpc::JsonRPCMessage MakeSuccessMessage(const std::string& text) {
  common::protocols::json_rpc::JsonRPCMessage msg;
  msg.result = text;
  return msg;
}

common::protocols::json_rpc::JsonRPCError MakeServerErrorFromText(const std::string& error_text) {
  common::protocols::json_rpc::JsonRPCError err;
  err.code = common::protocols::json_rpc::JSON_RPC_SERVER_ERROR;
  err.message = error_text;
  return err;
}

common::protocols::json_rpc::JsonRPCError MakeInternalErrorFromText(const std::string& error_text) {
  common::protocols::json_rpc::JsonRPCError err;
  err.code = common::protocols::json_rpc::JSON_RPC_INTERNAL_ERROR;
  err.message = error_text;
  return err;
}

}  // namespace protocol
}  // namespace iptv_cloud
