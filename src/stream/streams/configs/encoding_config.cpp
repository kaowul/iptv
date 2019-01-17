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

#include "stream/streams/configs/encoding_config.h"

#include <string>

#include "constants.h"
#include "gst_constants.h"

#include "stream/gst_types.h"

#define DEFAULT_VIDEO_ENCODER X264_ENC
#define DEFAULT_AUDIO_ENCODER FAAC

namespace iptv_cloud {
namespace stream {
namespace streams {

const common::media::Rational kDefaultAspectRatio = {INVALID_ASPECT_RATIO_NUM, INVALID_ASPECT_RATIO_DEN};

EncodingConfig::EncodingConfig(const base_class& config)
    : base_class(config),
      deinterlace_(DEFAULT_DEINTERLACE),
      frame_rate_(INVALID_FRAME_RATE),
      volume_(DEFAULT_VOLUME),
      video_encoder_(DEFAULT_VIDEO_ENCODER),
      audio_encoder_(DEFAULT_AUDIO_ENCODER),
      audio_channels_count_(INVALID_AUDIO_CHANNELS_COUNT),
      video_encoder_args_(),
      video_encoder_str_args_(),
      size_(),
      video_bit_rate_(INVALID_VIDEO_BIT_RATE),
      audio_bit_rate_(INVALID_AUDIO_BIT_RATE),
      logo_(),
      decklink_video_mode_(DEFAULT_DECKLINK_VIDEO_MODE),
      aspect_ratio_(kDefaultAspectRatio) {}

void EncodingConfig::SetVolume(volume_t volume) {
  volume_ = volume;
}

volume_t EncodingConfig::GetVolume() const {
  return volume_;
}

int EncodingConfig::GetFramerate() const {
  return frame_rate_;
}

void EncodingConfig::SetFrameRate(int rate) {
  frame_rate_ = rate;
}

bool EncodingConfig::GetDeinterlace() const {
  return deinterlace_;
}

void EncodingConfig::SetDeinterlace(bool deinterlace) {
  deinterlace_ = deinterlace;
}

std::string EncodingConfig::GetVideoEncoder() const {
  return video_encoder_;
}

void EncodingConfig::SetVideoEncoder(const std::string& enc) {
  video_encoder_ = enc;
}

std::string EncodingConfig::GetAudioEncoder() const {
  return audio_encoder_;
}

void EncodingConfig::SetAudioEncoder(const std::string& enc) {
  audio_encoder_ = enc;
}

bool EncodingConfig::IsGpu() const {
  const std::string video_enc = GetVideoEncoder();
  EncoderType enc;
  if (GetEncoderType(video_enc, &enc)) {
    return enc == GPU_MFX || enc == GPU_VAAPI;
  }

  return false;
}

bool EncodingConfig::IsMfxGpu() const {
  const std::string video_enc = GetVideoEncoder();
  EncoderType enc;
  if (GetEncoderType(video_enc, &enc)) {
    return enc == GPU_MFX;
  }

  return false;
}

audio_channels_count_t EncodingConfig::GetAudioChannelsCount() const {
  return audio_channels_count_;
}

void EncodingConfig::SetAudioChannelsCount(audio_channels_count_t channels) {
  audio_channels_count_ = channels;
}

common::draw::Size EncodingConfig::GetSize() const {
  return size_;
}

void EncodingConfig::SetSize(common::draw::Size size) {
  size_ = size;
}

int EncodingConfig::GetVideoBitrate() const {
  return video_bit_rate_;
}

void EncodingConfig::SetVideoBitrate(int bitr) {
  video_bit_rate_ = bitr;
}

int EncodingConfig::GetAudioBitrate() const {
  return audio_bit_rate_;
}

void EncodingConfig::SetAudioBitrate(int bitr) {
  audio_bit_rate_ = bitr;
}

video_encoders_args_t EncodingConfig::GetVideoEncoderArgs() const {
  return video_encoder_args_;
}

void EncodingConfig::SetVideoEncoderArgs(const video_encoders_args_t& args) {
  video_encoder_args_ = args;
}

video_encoders_str_args_t EncodingConfig::GetVideoEncoderStrArgs() const {
  return video_encoder_str_args_;
}

void EncodingConfig::SetVideoEncoderStrArgs(const video_encoders_str_args_t& args) {
  video_encoder_str_args_ = args;
}

void EncodingConfig::SetLogo(const Logo& logo) {
  logo_ = logo;
}

Logo EncodingConfig::GetLogo() const {
  return logo_;
}

common::media::Rational EncodingConfig::GetAspectRatio() const {
  return aspect_ratio_;
}

void EncodingConfig::SetAspectRatio(common::media::Rational rat) {
  aspect_ratio_ = rat;
}

decklink_video_mode_t EncodingConfig::GetDecklinkMode() const {
  return decklink_video_mode_;
}

void EncodingConfig::SetDecklinkMode(decklink_video_mode_t decl) {
  decklink_video_mode_ = decl;
}

}  // namespace streams
}  // namespace stream
}  // namespace iptv_cloud
