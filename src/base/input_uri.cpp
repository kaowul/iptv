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

#include "base/input_uri.h"

#include <string>

#include <json-c/json_object.h>
#include <json-c/json_tokener.h>

#include "base/constants.h"

#define FIELD_INPUT_ID "id"
#define FIELD_INPUT_URI "uri"

#define FIELD_INPUT_RELAY_AUDIO "relay_audio"
#define FIELD_INPUT_RELAY_VIDEO "relay_video"
#define FIELD_INPUT_MUTE_AUDIO "mute"
#define FIELD_INPUT_VOLUME_AUDIO "volume"

namespace iptv_cloud {

InputUri::InputUri() : InputUri(0, common::uri::Url()) {}

InputUri::InputUri(uri_id_t id, const common::uri::Url& input)
    : base_class(), id_(id), input_(input), volume_(), mute_(false), relay_video_(false), relay_audio_(false) {}

bool InputUri::GetRelayVideo() const {
  return relay_video_;
}

void InputUri::SetRelayVideo(bool rv) {
  relay_video_ = rv;
}

bool InputUri::GetRelayAudio() const {
  return relay_audio_;
}

void InputUri::SetRelayAudio(bool ra) {
  relay_audio_ = ra;
}

InputUri::uri_id_t InputUri::GetID() const {
  return id_;
}

void InputUri::SetID(uri_id_t id) {
  id_ = id;
}

common::uri::Url InputUri::GetInput() const {
  return input_;
}

void InputUri::SetInput(const common::uri::Url& uri) {
  input_ = uri;
}

bool InputUri::GetMute() const {
  return mute_;
}

void InputUri::SetMute(bool mute) {
  mute_ = mute;
}

volume_t InputUri::GetVolume() const {
  return volume_;
}

void InputUri::SetVolume(volume_t vol) {
  volume_ = vol;
}

bool InputUri::Equals(const InputUri& inf) const {
  return id_ == inf.id_ && input_ == inf.input_ && volume_ == inf.volume_ && mute_ == inf.mute_ &&
         relay_video_ == inf.relay_video_ && relay_audio_ == inf.relay_audio_;
}

common::Error InputUri::DoDeSerialize(json_object* serialized) {
  InputUri res;
  json_object* jid = nullptr;
  json_bool jid_exists = json_object_object_get_ex(serialized, FIELD_INPUT_ID, &jid);
  if (jid_exists) {
    res.SetID(json_object_get_int64(jid));
  }

  json_object* juri = nullptr;
  json_bool juri_exists = json_object_object_get_ex(serialized, FIELD_INPUT_URI, &juri);
  if (juri_exists) {
    res.SetInput(common::uri::Url(json_object_get_string(juri)));
  }

  json_object* juri_volume_audio = nullptr;
  json_bool juri_volume_audio_exists =
      json_object_object_get_ex(serialized, FIELD_INPUT_VOLUME_AUDIO, &juri_volume_audio);
  if (juri_volume_audio_exists) {
    res.SetVolume(json_object_get_double(juri_volume_audio));
  }

  json_object* juri_mute_audio = nullptr;
  json_bool juri_mute_audio_exists = json_object_object_get_ex(serialized, FIELD_INPUT_MUTE_AUDIO, &juri_mute_audio);
  if (juri_mute_audio_exists) {
    res.SetMute(json_object_get_boolean(juri_mute_audio));
  }

  json_object* juri_relay_video = nullptr;
  json_bool juri_relay_video_exists = json_object_object_get_ex(serialized, FIELD_INPUT_RELAY_VIDEO, &juri_relay_video);
  if (juri_relay_video_exists) {
    res.SetRelayVideo(json_object_get_boolean(juri_relay_video));
  }

  json_object* juri_relay_audio = nullptr;
  json_bool juri_relay_audio_exists = json_object_object_get_ex(serialized, FIELD_INPUT_RELAY_AUDIO, &juri_relay_audio);
  if (juri_relay_audio_exists) {
    res.SetRelayAudio(json_object_get_boolean(juri_relay_audio));
  }

  *this = res;
  return common::Error();
}

common::Error InputUri::SerializeFields(json_object* out) const {
  json_object_object_add(out, FIELD_INPUT_ID, json_object_new_int64(GetID()));
  std::string url_str = common::ConvertToString(GetInput());
  json_object_object_add(out, FIELD_INPUT_URI, json_object_new_string(url_str.c_str()));
  const auto vol = GetVolume();
  if (vol) {
    json_object_object_add(out, FIELD_INPUT_VOLUME_AUDIO, json_object_new_double(*vol));
  }
  json_object_object_add(out, FIELD_INPUT_MUTE_AUDIO, json_object_new_boolean(GetMute()));
  json_object_object_add(out, FIELD_INPUT_RELAY_VIDEO, json_object_new_boolean(GetRelayVideo()));
  json_object_object_add(out, FIELD_INPUT_RELAY_AUDIO, json_object_new_boolean(GetRelayAudio()));
  return common::Error();
}

bool IsTestInputUrl(const InputUri& url) {
  return url.GetInput() == common::uri::Url(TEST_URL);
}

}  // namespace iptv_cloud
