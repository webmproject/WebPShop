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
#include "webp/decode.h"

bool DecodeOneImage(const WebPData& encoded_data,
                    ImageMemoryDesc* const compressed_image) {
  START_TIMER(DecodeOneImage);

  if (encoded_data.bytes == nullptr || compressed_image == nullptr) {
    LOG("/!\\ Source or destination is null.");
    return false;
  }

  WebPDecoderConfig decoder_config;
  VP8StatusCode status;
  if (!WebPInitDecoderConfig(&decoder_config)) {
    LOG("/!\\ WebPInitDecoderConfig failed.");
    return false;
  }
  if ((status = WebPGetFeatures(encoded_data.bytes, encoded_data.size,
                                &decoder_config.input)) != VP8_STATUS_OK) {
    LOG("/!\\ WebPGetFeatures failed (" << status << ")");
    return false;
  }
  if (!AllocateImage(compressed_image, decoder_config.input.width,
                     decoder_config.input.height, 4)) {
    LOG("/!\\ AllocateImage failed.");
    return false;
  }

  // Will always be RGBA since we force 4 channels.
  decoder_config.output.colorspace = MODE_RGBA;
  decoder_config.output.u.RGBA.rgba = (uint8_t*)compressed_image->pixels.data;
  decoder_config.output.u.RGBA.stride =
      (int)(compressed_image->pixels.rowBits / 8);
  decoder_config.output.u.RGBA.size = (size_t)(
      decoder_config.output.u.RGBA.stride * decoder_config.input.height);
  decoder_config.output.is_external_memory = 1;
  decoder_config.output.width = decoder_config.input.width;
  decoder_config.output.height = decoder_config.input.height;

  if ((status = WebPDecode(encoded_data.bytes, encoded_data.size,
                           &decoder_config)) != VP8_STATUS_OK) {
    LOG("/!\\ WebPDecode failed (" << status << ")");
    WebPFreeDecBuffer(&decoder_config.output);
    return false;
  }
  WebPFreeDecBuffer(&decoder_config.output);
  LOG("Decoded " << encoded_data.size << " bytes.");

  STOP_TIMER(DecodeOneImage);
  return true;
}
