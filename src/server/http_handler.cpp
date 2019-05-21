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

#include "server/http_handler.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>
#include <utility>

#include "server/http_client.h"

namespace iptv_cloud {
namespace server {

HttpHandler::HttpHandler() : http_root_(http_directory_path_t::MakeHomeDir()) {}

void HttpHandler::SetHttpRoot(const http_directory_path_t& http_root) {
  http_root_ = http_root;
}

void HttpHandler::PreLooped(common::libev::IoLoop* server) {
  UNUSED(server);
}

void HttpHandler::Accepted(common::libev::IoClient* client) {
  UNUSED(client);
}

void HttpHandler::Moved(common::libev::IoLoop* server, common::libev::IoClient* client) {
  UNUSED(server);
  UNUSED(client);
}

void HttpHandler::Closed(common::libev::IoClient* client) {
  UNUSED(client);
}

void HttpHandler::TimerEmited(common::libev::IoLoop* server, common::libev::timer_id_t id) {
  UNUSED(server);
  UNUSED(id);
}

#if LIBEV_CHILD_ENABLE
void HttpHandler::Accepted(common::libev::IoChild* child) {
  UNUSED(child);
}

void HttpHandler::Moved(common::libev::IoLoop* server, common::libev::IoChild* child) {
  UNUSED(server);
  UNUSED(child);
}

void HttpHandler::ChildStatusChanged(common::libev::IoChild* child, int status) {
  UNUSED(child);
  UNUSED(status);
}
#endif

void HttpHandler::DataReceived(common::libev::IoClient* client) {
  char buff[BUF_SIZE] = {0};
  size_t nread = 0;
  common::ErrnoError errn = client->SingleRead(buff, BUF_SIZE - 1, &nread);
  if ((errn && errn->GetErrorCode() != EAGAIN) || nread == 0) {
    client->Close();
    delete client;
    return;
  }

  HttpClient* hclient = static_cast<server::HttpClient*>(client);
  ProcessReceived(hclient, buff, nread);
}

void HttpHandler::DataReadyToWrite(common::libev::IoClient* client) {
  UNUSED(client);
}

void HttpHandler::PostLooped(common::libev::IoLoop* server) {
  UNUSED(server);
}

void HttpHandler::ProcessReceived(HttpClient* hclient, const char* request, size_t req_len) {
  static const common::libev::http::HttpServerInfo hinf(PROJECT_NAME_TITLE, PROJECT_DOMAIN);
  common::http::HttpRequest hrequest;
  std::string request_str(request, req_len);
  std::pair<common::http::http_status, common::Error> result = common::http::parse_http_request(request_str, &hrequest);

  if (result.second) {
    const std::string error_text = result.second->GetDescription();
    DEBUG_MSG_ERROR(result.second, common::logging::LOG_LEVEL_ERR);
    common::ErrnoError err =
        hclient->SendError(common::http::HP_1_1, result.first, nullptr, error_text.c_str(), false, hinf);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    }
    hclient->Close();
    delete hclient;
    return;
  }

  // keep alive
  common::http::header_t connectionField;
  hrequest.FindHeaderByKey("Connection", false, &connectionField);
  bool IsKeepAlive = common::EqualsASCII(connectionField.value, "Keep-Alive", false);
  const common::http::http_protocol protocol = hrequest.GetProtocol();
  const char* extra_header = nullptr;
  if (hrequest.GetMethod() == common::http::http_method::HM_GET ||
      hrequest.GetMethod() == common::http::http_method::HM_HEAD) {
    common::uri::Upath path = hrequest.GetPath();
    if (!path.IsValid() || path.IsRoot()) {  // for hls
      common::ErrnoError err =
          hclient->SendError(protocol, common::http::HS_NOT_FOUND, extra_header, "File not found.", IsKeepAlive, hinf);
      if (err) {
        DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      }
      return;
    }

    const std::string url_dirs = path.GetHpath();
    auto dirs_path = http_root_.MakeDirectoryStringPath(url_dirs.substr(1));
    if (!dirs_path) {
      dirs_path = http_root_;
    }

    auto file_path = dirs_path->MakeFileStringPath(path.GetFileName());
    if (!file_path) {
      common::ErrnoError err =
          hclient->SendError(protocol, common::http::HS_NOT_FOUND, extra_header, "File not found.", IsKeepAlive, hinf);
      if (err) {
        DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      }
      return;
    }

    const std::string file_path_str = file_path->GetPath();
    INFO_LOG() << "Http file request path: " << file_path_str;
    int open_flags = O_RDONLY;
    struct stat sb;
    if (stat(file_path_str.c_str(), &sb) < 0) {
      common::ErrnoError err =
          hclient->SendError(protocol, common::http::HS_NOT_FOUND, extra_header, "File not found.", IsKeepAlive, hinf);
      if (err) {
        DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      }
      return;
    }

    if (S_ISDIR(sb.st_mode)) {
      common::ErrnoError err =
          hclient->SendError(protocol, common::http::HS_BAD_REQUEST, extra_header, "Bad filename.", IsKeepAlive, hinf);
      if (err) {
        DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      }
      return;
    }

    int file = open(file_path_str.c_str(), open_flags);
    if (file == INVALID_DESCRIPTOR) { /* open the file for reading */
      common::ErrnoError err = hclient->SendError(protocol, common::http::HS_FORBIDDEN, extra_header,
                                                  "File is protected.", IsKeepAlive, hinf);
      if (err) {
        DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      }
      return;
    }

    const std::string mime = path.GetMime();
    common::ErrnoError err = hclient->SendHeaders(protocol, common::http::HS_OK, extra_header, mime.c_str(),
                                                  &sb.st_size, &sb.st_mtime, IsKeepAlive, hinf);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      ::close(file);
      return;
    }

    if (hrequest.GetMethod() == common::http::http_method::HM_GET) {
      common::ErrnoError err = hclient->SendFileByFd(protocol, file, sb.st_size);
      if (err) {
        DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      }
    }
    ::close(file);
    return;
  }

  if (!IsKeepAlive) {
    hclient->Close();
    delete hclient;
  }
}

}  // namespace server
}  // namespace iptv_cloud
