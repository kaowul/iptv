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

#include <common/protocols/json_rpc/json_rpc.h>

#define OK_RESULT "OK"

namespace iptv_cloud {
namespace protocol {

typedef common::protocols::json_rpc::JsonRPCResponce responce_t;
typedef common::protocols::json_rpc::JsonRPCRequest request_t;
typedef common::protocols::json_rpc::json_rpc_request_params params_t;
typedef std::string sequance_id_t;
typedef std::string serializet_t;

common::protocols::json_rpc::JsonRPCMessage MakeSuccessMessage(const std::string& text = OK_RESULT);
common::protocols::json_rpc::JsonRPCError MakeServerErrorFromText(const std::string& error_text);
common::protocols::json_rpc::JsonRPCError MakeInternalErrorFromText(const std::string& error_text);

}  // namespace protocol
}  // namespace iptv_cloud
