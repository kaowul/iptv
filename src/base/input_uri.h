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

#include <common/serializer/json_serializer.h>
#include <common/uri/url.h>

#include "base/types.h"

namespace iptv_cloud {

class InputUri : public common::serializer::JsonSerializer<InputUri> {
 public:
  typedef JsonSerializer<InputUri> base_class;
  typedef channel_id_t uri_id_t;

  InputUri();
  explicit InputUri(uri_id_t id, const common::uri::Url& input);

  bool GetRelayVideo() const;
  void SetRelayVideo(bool rv);

  bool GetRelayAudio() const;
  void SetRelayAudio(bool ra);

  uri_id_t GetID() const;
  void SetID(uri_id_t id);

  common::uri::Url GetInput() const;
  void SetInput(const common::uri::Url& uri);

  volume_t GetVolume() const;  // 0.0, 10.0
  void SetVolume(volume_t vol);

  bool Equals(const InputUri& inf) const;

 protected:
  common::Error DoDeSerialize(json_object* serialized) override;
  common::Error SerializeFields(json_object* out) const override;

 private:
  uri_id_t id_;
  common::uri::Url input_;

  volume_t volume_;

  bool relay_video_;
  bool relay_audio_;
};

inline bool operator==(const InputUri& left, const InputUri& right) {
  return left.Equals(right);
}

inline bool operator!=(const InputUri& left, const InputUri& right) {
  return !operator==(left, right);
}

bool IsTestInputUrl(const InputUri& url);

}  // namespace iptv_cloud
