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

#include "stream/streams_factory.h"

#include <string>

#include "config_fields.h"

#include "stream/streams/catchup_stream.h"
#include "stream/streams/device_stream.h"
#include "stream/streams/encoding_only_audio_stream.h"
#include "stream/streams/encoding_only_video_stream.h"
#include "stream/streams/mosaic_stream.h"
#include "stream/streams/playlist_encoding_stream.h"
#include "stream/streams/playlist_relay_stream.h"
#include "stream/streams/test_stream.h"
#include "stream/streams/timeshift_player_stream.h"
#include "stream/streams/timeshift_recorder_stream.h"

#include "utils/arg_converter.h"

namespace iptv_cloud {
namespace stream {

IBaseStream* StreamsFactory::CreateStream(const Config* config,
                                          IBaseStream::IStreamClient* client,
                                          StreamStruct* stats,
                                          const TimeShiftInfo& tinfo,
                                          chunk_index_t start_chunk_index) {
  input_t input = config->GetInput();
  if (config->GetType() == RELAY) {
    const streams::RelayConfig* rconfig = static_cast<const streams::RelayConfig*>(config);
    InputUri iuri = input[0];
    common::uri::Url input_uri = iuri.GetInput();
    if (input_uri.GetScheme() == common::uri::Url::file) {  // multi input or playlist
      const streams::PlaylistRelayConfig* prconfig = static_cast<const streams::PlaylistRelayConfig*>(config);
      return new streams::PlaylistRelayStream(prconfig, client, stats);
    }

    /*if (input.size() > 1) {
      return new streams::MosaicStream(rconfig, client, stats);
    }*/

    return new streams::RelayStream(rconfig, client, stats);
  } else if (config->GetType() == ENCODE) {
    const streams::EncodingConfig* econfig = static_cast<const streams::EncodingConfig*>(config);
    InputUri iuri = input[0];
    common::uri::Url input_uri = iuri.GetInput();
    if (input_uri.GetScheme() == common::uri::Url::file) {  // multi input or playlist
      return new streams::PlaylistEncodingStream(econfig, client, stats);
    }

    if (input.size() > 1) {
      return new streams::MosaicStream(econfig, client, stats);
    }

    if (IsTestUrl(iuri)) {
      return new streams::TestStream(econfig, client, stats);
    }

    if (input_uri.GetScheme() == common::uri::Url::dev) {
      return new streams::DeviceStream(econfig, client, stats);
    }

    if (iuri.GetRelayVideo()) {
      return new streams::EncodingOnlyAudioStream(econfig, client, stats);
    } else if (iuri.GetRelayAudio()) {
      return new streams::EncodingOnlyVideoStream(econfig, client, stats);
    }

    return new streams::EncodingStream(econfig, client, stats);
  } else if (config->GetType() == TIMESHIFT_PLAYER) {
    const streams::RelayConfig* tconfig = static_cast<const streams::RelayConfig*>(config);
    return new streams::TimeShiftPlayerStream(tconfig, tinfo, client, stats, start_chunk_index);
  } else if (config->GetType() == TIMESHIFT_RECORDER) {
    const streams::TimeshiftConfig* tconfig = static_cast<const streams::TimeshiftConfig*>(config);
    return new streams::TimeShiftRecorderStream(tconfig, tinfo, client, stats);
  } else if (config->GetType() == CATCHUP) {
    const streams::TimeshiftConfig* tconfig = static_cast<const streams::TimeshiftConfig*>(config);
    return new streams::CatchupStream(tconfig, tinfo, client, stats);
  }

  NOTREACHED();
  return nullptr;
}

}  // namespace stream
}  // namespace iptv_cloud
