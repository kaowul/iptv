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

#include "output_uri.h"

#include <json-c/json_object.h>
#include <json-c/json_tokener.h>

#include <common/sprintf.h>

#include "constants.h"

#define FIELD_OUTPUT_ID "id"
#define FIELD_OUTPUT_URI "uri"
#define FIELD_OUTPUT_HTTP_ROOT "http_root"
#define FIELD_OUTPUT_SIZE "size"
#define FIELD_OUTPUT_VIDEO_BITRATE "video_bitrate"
#define FIELD_OUTPUT_AUDIO_BITRATE "audio_bitrate"

namespace iptv_cloud {

OutputUri::OutputUri() : OutputUri(0, common::uri::Url()) {}

OutputUri::OutputUri(stream_id_t id, const common::uri::Url& output)
    : id_(id),
      output_(output),
      http_root_(),
      size_(),
      audio_bit_rate_(INVALID_AUDIO_BIT_RATE),
      video_bit_rate_(INVALID_VIDEO_BIT_RATE) {}

stream_id_t OutputUri::GetID() const {
  return id_;
}

void OutputUri::SetID(stream_id_t id) {
  id_ = id;
}

common::uri::Url OutputUri::GetOutput() const {
  return output_;
}

void OutputUri::SetOutput(const common::uri::Url& uri) {
  output_ = uri;
}

OutputUri::http_root_t OutputUri::GetHttpRoot() const {
  return http_root_;
}

void OutputUri::SetHttpRoot(const http_root_t& root) {
  http_root_ = root;
}

common::draw::Size OutputUri::GetSize() const {
  return size_;
}

void OutputUri::SetSize(common::draw::Size size) {
  size_ = size;
}

int OutputUri::GetAudioBitrate() const {
  return audio_bit_rate_;
}

void OutputUri::SetAudioBitrate(int rate) {
  audio_bit_rate_ = rate;
}

int OutputUri::GetVideoBitrate() const {
  return video_bit_rate_;
}

void OutputUri::SetVideoBitrate(int rate) {
  video_bit_rate_ = rate;
}

bool OutputUri::Equals(const OutputUri& inf) const {
  return id_ == inf.id_ && output_ == inf.output_ && http_root_ == inf.http_root_ && size_ == inf.size_ &&
         audio_bit_rate_ == inf.audio_bit_rate_ && video_bit_rate_ == inf.video_bit_rate_;
}

}  // namespace iptv_cloud

namespace common {

namespace {
const std::string hls_type_str[] = {"none", "pull", "push"};
const std::string hls_out_type_str[] = {"main", "adaptive"};
}  // namespace

std::string ConvertToString(const iptv_cloud::OutputUri& value) {
  common::file_system::ascii_directory_string_path ps = value.GetHttpRoot();
  const std::string http_root_str = ps.GetPath();
  return common::MemSPrintf("{ \"" FIELD_OUTPUT_ID "\": %llu, \"" FIELD_OUTPUT_URI
                            "\": \"%s\", \"" FIELD_OUTPUT_HTTP_ROOT "\": \"%s\", \"" FIELD_OUTPUT_SIZE
                            "\": \"%s\", \"" FIELD_OUTPUT_VIDEO_BITRATE "\": %d, \"" FIELD_OUTPUT_AUDIO_BITRATE
                            "\": %d }",
                            value.GetID(), common::ConvertToString(value.GetOutput()), http_root_str,
                            common::ConvertToString(value.GetSize()), value.GetVideoBitrate(), value.GetAudioBitrate());
}

bool ConvertFromString(const std::string& from, iptv_cloud::OutputUri* out) {
  if (!out) {
    return false;
  }

  json_object* obj = json_tokener_parse(from.c_str());
  if (!obj) {
    return false;
  }

  iptv_cloud::OutputUri res;
  json_object* jid = nullptr;
  json_bool jid_exists = json_object_object_get_ex(obj, FIELD_OUTPUT_ID, &jid);
  if (jid_exists) {
    res.SetID(json_object_get_int64(jid));
  }

  json_object* juri = nullptr;
  json_bool juri_exists = json_object_object_get_ex(obj, FIELD_OUTPUT_URI, &juri);
  if (juri_exists) {
    res.SetOutput(common::uri::Url(json_object_get_string(juri)));
  }

  json_object* jhttp_root = nullptr;
  json_bool jhttp_root_exists = json_object_object_get_ex(obj, FIELD_OUTPUT_HTTP_ROOT, &jhttp_root);
  if (jhttp_root_exists) {
    const char* http_root_str = json_object_get_string(jhttp_root);
    const common::file_system::ascii_directory_string_path http_root(http_root_str);
    res.SetHttpRoot(http_root);
  }

  json_object* jsize = nullptr;
  json_bool jsize_exists = json_object_object_get_ex(obj, FIELD_OUTPUT_SIZE, &jsize);
  if (jsize_exists) {
    common::draw::Size size;
    if (common::ConvertFromString(json_object_get_string(jsize), &size)) {
      res.SetSize(size);
    }
  }

  json_object* jvbitrate = nullptr;
  json_bool jvbitrate_exists = json_object_object_get_ex(obj, FIELD_OUTPUT_VIDEO_BITRATE, &jvbitrate);
  if (jvbitrate_exists) {
    res.SetVideoBitrate(json_object_get_int(jvbitrate));
  }

  json_object* jabitrate = nullptr;
  json_bool jabitrate_exists = json_object_object_get_ex(obj, FIELD_OUTPUT_AUDIO_BITRATE, &jabitrate);
  if (jabitrate_exists) {
    res.SetAudioBitrate(json_object_get_int(jabitrate));
  }

  json_object_put(obj);
  *out = res;
  return true;
}

}  // namespace common
