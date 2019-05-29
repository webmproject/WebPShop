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

#include "FileUtilities.h"
#include "WebPShop.h"
#include "webp/encode.h"

void SetWebPConfig(WebPConfig* const config, const WriteConfig& write_config) {
  if (write_config.quality < 0 || write_config.quality > 100) {
    LOG("/!\\ Bad write_config.");
    return;
  }

  const int near_lossless_starts_at = 98;
  if (write_config.quality >= near_lossless_starts_at) {
    config->lossless = 1;
    config->near_lossless = (write_config.quality == 98) ? 60 :
                            (write_config.quality == 99) ? 80 :
                                                           100;
    if (write_config.compression == Compression::FASTEST) {
      config->quality = 0.0f;
    } else if (write_config.compression == Compression::DEFAULT) {
      config->quality = 75.0f;
    } else {
      config->quality = 100.0f;
    }
  } else {
    config->lossless = 0;
    config->quality =
        write_config.quality * 100.0f / (near_lossless_starts_at - 1);
    config->use_sharp_yuv = (write_config.compression == Compression::SLOWEST);
  }

  if (write_config.compression == Compression::FASTEST) {
    config->method = 1;
  } else if (write_config.compression == Compression::DEFAULT) {
    config->method = 4;
  } else {
    config->method = 6;
  }

  // Low alpha qualities are terrible on gradients: limit to acceptable range.
  config->alpha_quality = 50 + write_config.quality / 2;
  if (config->alpha_quality > 100) config->alpha_quality = 100;
}

bool EncodeOneImage(const ImageMemoryDesc& original_image,
                    const WriteConfig& write_config,
                    WebPData* const encoded_data) {
  START_TIMER(EncodeOneImage);

  if (original_image.pixels.data == nullptr || original_image.width < 1 ||
      original_image.height < 1 || original_image.num_channels != 4 ||
      encoded_data == nullptr) {
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

  pic.use_argb = 1;
  pic.width = original_image.width;
  pic.height = original_image.height;
  pic.argb_stride = pic.width;
  // Will not be modified (because no show_compressed).
  pic.argb = (uint32_t*)original_image.pixels.data;

  START_TIMER(WebPEncode);
  WebPMemoryWriter memory_writer;
  WebPMemoryWriterInit(&memory_writer);
  pic.writer = WebPMemoryWrite;
  pic.custom_ptr = &memory_writer;
  if (!WebPEncode(&config, &pic)) {
    LOG("/!\\ WebPEncode failed (" << pic.error_code << ").");
    WebPPictureFree(&pic);
    return false;
  }
  STOP_TIMER(WebPEncode);

  WebPPictureFree(&pic);
  WebPDataClear(encoded_data);
  encoded_data->bytes = memory_writer.mem;
  encoded_data->size = memory_writer.size;
  // Do not clear memory_writer's data.
  LOG("Encoded " << encoded_data->size << " bytes.");

  STOP_TIMER(EncodeOneImage);
  return true;
}

void WriteToFile(const WebPData& encoded_data, FormatRecordPtr format_record,
                 int16* const result) {
  START_TIMER(WriteToFile);

  if (encoded_data.bytes == nullptr || encoded_data.size == 0) {
    LOG("/!\\ Source is null.");
    return;
  }

  if (*result == noErr) {
    // Move cursor to the start of the output file.
    *result = PSSDKSetFPos((int32)format_record->dataFork,
                           format_record->posixFileDescriptor,
                           format_record->pluginUsingPOSIXIO, fsFromStart, 0);
  }

  if (*result == noErr) {
    LOG("Writing " << encoded_data.size << " bytes.");
    WriteSome(encoded_data.size, encoded_data.bytes, format_record, result);
  }

  STOP_TIMER(WriteToFile);
}
