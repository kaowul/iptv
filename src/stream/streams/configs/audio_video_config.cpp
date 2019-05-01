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

#include "stream/streams/configs/audio_video_config.h"

#include "base/constants.h"

namespace iptv_cloud {
namespace stream {
namespace streams {

AudioVideoConfig::AudioVideoConfig(const base_class& config)
    : base_class(config), have_video_(), have_audio_(), audio_select_(), loop_() {}

AudioVideoConfig::have_stream_t AudioVideoConfig::HaveVideo() const {
  return have_video_;
}

void AudioVideoConfig::SetHaveVideo(have_stream_t have_video) {
  have_video_ = have_video;
}

AudioVideoConfig::have_stream_t AudioVideoConfig::HaveAudio() const {
  return have_audio_;
}

void AudioVideoConfig::SetHaveAudio(have_stream_t have_audio) {
  have_audio_ = have_audio;
}

AudioVideoConfig::audio_select_t AudioVideoConfig::GetAudioSelect() const {
  return audio_select_;
}

void AudioVideoConfig::SetAudioSelect(audio_select_t sel) {
  audio_select_ = sel;
}

AudioVideoConfig::loop_t AudioVideoConfig::GetLoop() const {
  return loop_;
}

void AudioVideoConfig::SetLoop(loop_t loop) {
  loop_ = loop;
}

}  // namespace streams
}  // namespace stream
}  // namespace iptv_cloud
