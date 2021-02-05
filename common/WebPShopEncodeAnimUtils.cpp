// Copyright 2018 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "WebPShop.h"
#include "webp/mux.h"

bool TryExtractDuration(const uint16* const layer_name,
                        int* const duration_ms) {
  if (layer_name == nullptr || duration_ms == nullptr) return false;
  *duration_ms = -1;

  const int max_num_browsed_characters = 256;   // Safety net.
  const int max_milliseconds = (1 << 30) / 10;  // No overflow.
  enum WaitingFor { openingParenthesis, number, ms, closingParenthesis };

  WaitingFor waiting_for = WaitingFor::openingParenthesis;
  int milliseconds = -1;
  for (int i = 0; layer_name[i] != 0 && i < max_num_browsed_characters; ++i) {
    if (layer_name[i] == ' ') continue;  // Ignore spaces.

    if (waiting_for == WaitingFor::number) {  // Parse positive integer.
      if (layer_name[i] >= '0' && layer_name[i] <= '9' &&
          milliseconds <= max_milliseconds) {
        if (milliseconds == -1) milliseconds = 0;  // First digit.
        milliseconds = milliseconds * 10 + (layer_name[i] - '0');
      } else if (milliseconds > -1) {  // We found a number.
        waiting_for = WaitingFor::ms;  // Carry on.
      } else {                         // We didn't find any digit.
        waiting_for = WaitingFor::openingParenthesis;  // Restart.
      }
    }

    if (waiting_for == WaitingFor::ms) {  // Parse case-insensitive 'ms'.
      if (layer_name[i] == 'M' || layer_name[i] == 'm') {
        ++i;
        if (layer_name[i] == 'S' || layer_name[i] == 's') {
          waiting_for = WaitingFor::closingParenthesis;  // Carry on.
        } else {
          waiting_for = WaitingFor::openingParenthesis;  // Restart.
        }
      } else {
        waiting_for = WaitingFor::openingParenthesis;  // Restart.
      }
    } else if (waiting_for == WaitingFor::closingParenthesis) {
      if (layer_name[i] == ')') {
        *duration_ms = milliseconds;  // Success.
      }
      waiting_for = WaitingFor::openingParenthesis;  // Restart.
    }

    if (waiting_for == WaitingFor::openingParenthesis) {
      if (layer_name[i] == '(') {
        milliseconds = -1;                 // No digit found yet.
        waiting_for = WaitingFor::number;  // Carry on.
      }
    }
  }
  return *duration_ms > -1;
}

//------------------------------------------------------------------------------

bool EncodeAllFrames(const std::vector<FrameMemoryDesc>& original_frames,
                     const WriteConfig& write_config,
                     WebPData* const encoded_data) {
  START_TIMER(EncodeAllFrames);

  if (original_frames.empty() || original_frames[0].image.width < 1 ||
      original_frames[0].image.height < 1 ||
      original_frames[0].image.num_channels != 4 || encoded_data == nullptr) {
    LOG("/!\\ Source is null or incompatible.");
    return false;
  }

  WebPConfig config;
  if (!WebPConfigInit(&config)) {
    LOG("/!\\ WebPConfigInit() failed.");
    return false;
  }
  SetWebPConfig(&config, write_config);

  WebPPicture pic;
  if (!WebPPictureInit(&pic)) {
    LOG("/!\\ WebPPictureInit() failed.");
    return false;
  }

  WebPAnimEncoderOptions anim_encoder_options;
  if (!WebPAnimEncoderOptionsInit(&anim_encoder_options)) {
    LOG("/!\\ WebPAnimEncoderOptionsInit() failed.");
    return false;
  }
  anim_encoder_options.anim_params.loop_count =
      write_config.loop_forever ? 0 : 1;

  WebPAnimEncoder* anim_encoder = WebPAnimEncoderNew(
      original_frames[0].image.width, original_frames[0].image.height,
      &anim_encoder_options);
  if (anim_encoder == nullptr) {
    LOG("/!\\ WebPAnimEncoderNew() failed.");
    return false;
  }

  int timestamp_ms = 0;

  for (size_t i = 0; i < original_frames.size(); ++i) {
    const FrameMemoryDesc& frame = original_frames[i];
    pic.use_argb = 1;
    pic.width = frame.image.width;
    pic.height = frame.image.height;
    pic.argb_stride = pic.width;
    pic.argb = (uint32_t*)frame.image.pixels.data;  // Will not be modified.

    if (!WebPAnimEncoderAdd(anim_encoder, &pic, timestamp_ms, &config)) {
      LOG("/!\\ WebPAnimEncoderAdd failed (" << pic.error_code << ").");
      WebPPictureFree(&pic);
      WebPAnimEncoderDelete(anim_encoder);
      return false;
    }

    timestamp_ms += frame.duration_ms;
  }

  WebPPictureFree(&pic);

  if (!WebPAnimEncoderAdd(anim_encoder, nullptr, timestamp_ms, nullptr)) {
    LOG("/!\\ Last WebPAnimEncoderAdd() failed.");
    WebPAnimEncoderDelete(anim_encoder);
    return false;
  }

  WebPDataClear(encoded_data);
  if (!WebPAnimEncoderAssemble(anim_encoder, encoded_data)) {
    LOG("/!\\ WebPAnimEncoderAssemble() failed.");
    WebPAnimEncoderDelete(anim_encoder);
    return false;
  }

  WebPAnimEncoderDelete(anim_encoder);
  LOG("Encoded " << original_frames.size() << " frames into "
                 << encoded_data->size << " bytes.");

  STOP_TIMER(EncodeAllFrames);
  return true;
}
