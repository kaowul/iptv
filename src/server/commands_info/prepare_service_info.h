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

#include <common/serializer/json_serializer.h>

#define PREPARE_SERVICE_INFO_FEEDBACK_DIRECTORY_FIELD "feedback_directory"
#define PREPARE_SERVICE_INFO_TIMESHIFTS_DIRECTORY_FIELD "timeshifts_directory"
#define PREPARE_SERVICE_INFO_HLS_DIRECTORY_FIELD "hls_directory"
#define PREPARE_SERVICE_INFO_PLAYLIST_DIRECTORY_FIELD "playlists_directory"
#define PREPARE_SERVICE_INFO_DVB_DIRECTORY_FIELD "dvb_directory"
#define PREPARE_SERVICE_INFO_CAPTURE_CARD_DIRECTORY_FIELD "capture_card_directory"

namespace iptv_cloud {
namespace server {

class PrepareServiceInfo : public common::serializer::JsonSerializer<PrepareServiceInfo> {
 public:
  typedef JsonSerializer<PrepareServiceInfo> base_class;
  PrepareServiceInfo();

  std::string GetFeedbackDirectory() const;
  std::string GetTimeshiftsDirectory() const;
  std::string GetHlsDirectory() const;
  std::string GetPlaylistsDirectory() const;
  std::string GetDvbDirectory() const;
  std::string GetCaptureDirectory() const;

 protected:
  common::Error DoDeSerialize(json_object* serialized) override;
  common::Error SerializeFields(json_object* out) const override;

 private:
  std::string feedback_directory_;
  std::string timeshifts_directory_;
  std::string hls_directory_;
  std::string playlists_directory_;
  std::string dvb_directory_;
  std::string capture_card_directory_;
};

}  // namespace server
}  // namespace iptv_cloud
