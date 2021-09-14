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

#include <algorithm>

#include "WebPShop.h"
#include "webp/demux.h"

void PrintDuration(int frame_index, int frame_duration, uint16 output[],
                   size_t output_max_length) {
  std::string layer_name = "Frame " + std::to_string(frame_index + 1) + " (" +
                           std::to_string(frame_duration) + " ms)";
  layer_name.push_back('\0');
  if (output == nullptr || output_max_length < layer_name.size()) {
    LOG("/!\\ output is null or too small.");
    return;
  }
  std::copy(layer_name.begin(), layer_name.end(), output);
}

//------------------------------------------------------------------------------

bool DecodeAllFrames(const WebPData& encoded_data,
                     std::vector<FrameMemoryDesc>* const compressed_frames) {
  START_TIMER(DecodeAllFrames);

  if (encoded_data.bytes == nullptr || compressed_frames == nullptr) {
    LOG("/!\\ Source or destination is null.");
    return false;
  }

  WebPAnimDecoderOptions options;
  if (!WebPAnimDecoderOptionsInit(&options)) {
    LOG("/!\\ WebPAnimDecoderOptionsInit() failed.");
    return false;
  }
  options.color_mode = MODE_RGBA;

  WebPAnimDecoder* const anim_decoder =
      WebPAnimDecoderNew(&encoded_data, &options);
  if (anim_decoder == nullptr) {
    LOG("/!\\ WebPAnimDecoderNew() failed.");
    return false;
  }

  WebPAnimInfo info;
  if (!WebPAnimDecoderGetInfo(anim_decoder, &info)) {
    LOG("/!\\ WebPAnimDecoderGetInfo() failed.");
    WebPAnimDecoderDelete(anim_decoder);
    return false;
  }

  if (info.frame_count > MAX_NUM_BROWSED_LAYERS) {
    LOG("/!\\ Too many layers.");
    WebPAnimDecoderDelete(anim_decoder);
    return false;
  }

  ResizeFrameVector(compressed_frames, info.frame_count);
  size_t frame_counter = 0;
  int last_frame_timestamp_ms = 0;
  while (WebPAnimDecoderHasMoreFrames(anim_decoder) &&
         frame_counter < compressed_frames->size()) {
    uint8_t* buf;
    int timestamp;
    if (!WebPAnimDecoderGetNext(anim_decoder, &buf, &timestamp)) {
      LOG("/!\\ WebPAnimDecoderGetNext() failed.");
      WebPAnimDecoderDelete(anim_decoder);
      return false;
    }

    FrameMemoryDesc& compressed_frame = (*compressed_frames)[frame_counter];
    compressed_frame.duration_ms = timestamp - last_frame_timestamp_ms;

    if (!AllocateImage(&compressed_frame.image, (int32)info.canvas_width,
                       (int32)info.canvas_height, /*num_channels=*/4,
                       /*bit_depth=*/8)) {
      LOG("/!\\ AllocateImage failed.");
      WebPAnimDecoderDelete(anim_decoder);
      return false;
    }

    // The output of WebPAnimDecoderGetNext() is valid only for a loop.
    memcpy(compressed_frame.image.pixels.data, buf,
           info.canvas_width * info.canvas_height * 4);

    last_frame_timestamp_ms = timestamp;
    ++frame_counter;
  }

  WebPAnimDecoderDelete(anim_decoder);
  LOG("Decoded " << encoded_data.size << " bytes into " << frame_counter
                 << " / " << info.frame_count << " frames.");

  STOP_TIMER(DecodeAllFrames);
  return (!compressed_frames->empty() &&
          compressed_frames->size() == frame_counter);
}
