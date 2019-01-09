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

#include "server/daemon_commands.h"

namespace iptv_cloud {
namespace server {

protocol::responce_t StopServiceResponceSuccess(protocol::sequance_id_t id) {
  return protocol::responce_t::MakeMessage(id, protocol::MakeSuccessMessage());
}

protocol::responce_t StopServiceResponceFail(protocol::sequance_id_t id, const std::string& error_text) {
  return protocol::responce_t::MakeError(id, protocol::MakeServerErrorFromText(error_text));
}

protocol::request_t StopServiceRequest(protocol::sequance_id_t id, protocol::serializet_params_t params) {
  protocol::request_t req;
  req.id = id;
  req.method = CLIENT_STOP_SERVICE;
  req.params = params;
  return req;
}

protocol::responce_t ActivateResponceSuccess(protocol::sequance_id_t id) {
  return protocol::responce_t::MakeMessage(id, protocol::MakeSuccessMessage());
}

protocol::responce_t ActivateResponceFail(protocol::sequance_id_t id, const std::string& error_text) {
  return protocol::responce_t::MakeError(id, protocol::MakeServerErrorFromText(error_text));
}

protocol::responce_t StateServiceResponce(protocol::sequance_id_t id, const std::string& result) {
  return protocol::responce_t::MakeMessage(id, protocol::MakeSuccessMessage(result));
}

protocol::responce_t StartStreamResponceSuccess(protocol::sequance_id_t id) {
  return protocol::responce_t::MakeMessage(id, protocol::MakeSuccessMessage());
}

protocol::responce_t StartStreamResponceFail(protocol::sequance_id_t id, const std::string& error_text) {
  return protocol::responce_t::MakeError(id, protocol::MakeServerErrorFromText(error_text));
}

protocol::responce_t StopStreamResponceSuccess(protocol::sequance_id_t id) {
  return protocol::responce_t::MakeMessage(id, protocol::MakeSuccessMessage());
}

protocol::responce_t StopStreamResponceFail(protocol::sequance_id_t id, const std::string& error_text) {
  return protocol::responce_t::MakeError(id, protocol::MakeServerErrorFromText(error_text));
}

protocol::responce_t RestartStreamResponceSuccess(protocol::sequance_id_t id) {
  return protocol::responce_t::MakeMessage(id, protocol::MakeSuccessMessage());
}

protocol::responce_t RestartStreamResponceFail(protocol::sequance_id_t id, const std::string& error_text) {
  return protocol::responce_t::MakeError(id, protocol::MakeServerErrorFromText(error_text));
}

protocol::request_t PingDaemonRequest(protocol::sequance_id_t id, protocol::serializet_params_t params) {
  protocol::request_t req;
  req.id = id;
  req.method = SERVER_PING;
  req.params = params;
  return req;
}

protocol::responce_t PingServiceResponce(protocol::sequance_id_t id, const std::string& result) {
  return protocol::responce_t::MakeMessage(id, protocol::MakeSuccessMessage(result));
}

protocol::responce_t PingServiceResponceFail(protocol::sequance_id_t id, const std::string& error_text) {
  return protocol::responce_t::MakeError(id, protocol::MakeServerErrorFromText(error_text));
}

protocol::request_t ChangedSourcesStreamBroadcast(protocol::serializet_params_t params) {
  return protocol::request_t::MakeNotification(CLIENT_CHANGED_SOURCES_STREAM, params);
}

protocol::request_t StatisitcStreamBroadcast(protocol::serializet_params_t params) {
  return protocol::request_t::MakeNotification(CLIENT_STATISTIC_STREAM, params);
}

}  // namespace server
}  // namespace iptv_cloud
