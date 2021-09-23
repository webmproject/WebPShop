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

#include <cassert>

#include "FileUtilities.h"
#include "PIProperties.h"
#include "WebPShop.h"
#include "webp/encode.h"
#include "webp/mux.h"

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

bool CastToWebPPicture(const WebPConfig& config, const ImageMemoryDesc& src,
                       WebPPicture* const dst) {
  WebPPictureFree(dst);

  if (src.pixels.data == nullptr || src.width < 1 || src.height < 1 ||
      src.num_channels != 4 || src.pixels.depth != 8) {
    LOG("/!\\ Unsupported ImageMemoryDesc layout.");
    return false;
  }

  dst->use_argb = 1;
  dst->width = src.width;
  dst->height = src.height;

  // The data can be mapped to a WebPPicture without allocation.
  assert(!config.show_compressed);  // 'dst->argb[]' will not be altered.
  dst->argb = const_cast<uint32_t*>(
      reinterpret_cast<const uint32_t*>(src.pixels.data));
  dst->argb_stride = src.width;
  return true;
}

bool EncodeOneImage(const ImageMemoryDesc& original_image,
                    const WriteConfig& write_config,
                    WebPData* const encoded_data) {
  START_TIMER(EncodeOneImage);

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

  if (!CastToWebPPicture(config, original_image, &pic)) {
    WebPPictureFree(&pic);
    return false;
  }

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

static OSErr GetHostProperty(PIType key, Metadata* const metadata) {
  OSErr result = noErr;

  // Code below is inspired from PIGetXMP() and PIGetEXIFData().
  if (sPSProperty->getPropertyProc == nullptr) {
    LOG("getPropertyProc is null, considering no " << metadata->four_cc);
  } else {
    Handle handle = nullptr;
    const OSErr error = sPSProperty->getPropertyProc(kPhotoshopSignature, key,
                                                     0, nullptr, &handle);
    if (error != noErr) {
      LOG("getPropertyProc failed (" << error << "), considering no "
                                     << metadata->four_cc);
    } else if (handle == nullptr) {
      LOG("handle is null, considering no " << metadata->four_cc);
    } else {
      const int32 size = sPSHandle->GetSize(handle);
      if (size <= 0) {
        LOG("size is " << size << ", considering no " << metadata->four_cc);
      } else {
        Boolean oldLock = FALSE;
        Ptr ptr = nullptr;
        sPSHandle->SetLock(handle, true, &ptr, &oldLock);
        if (ptr == nullptr) {
          LOG("/!\\ SetLock failed");
          result = vLckdErr;
        } else {
          LOG("Got " << size << " bytes of " << metadata->four_cc);
          const WebPData view = {(const uint8_t*)ptr, (size_t)size};
          if (!WebPDataCopy(&view, &metadata->chunk)) {
            LOG("/!\\ WebPDataCopy of " << metadata->four_cc << " chunk ("
                                        << size << " bytes) failed.");
            result = memFullErr;
          }
          sPSHandle->SetLock(handle, false, &ptr, &oldLock);
        }
      }
      sPSHandle->Dispose(handle);
    }
  }
  return result;
}

static OSErr GetHostICCProfile(FormatRecordPtr format_record,
                               Metadata* const metadata) {
  OSErr result = noErr;
  if (format_record->handleProcs == nullptr) {
    LOG("handleProcs is null, considering no ICCP");
  } else if (format_record->handleProcs->lockProc == nullptr) {
    LOG("handleProcs->lockProc is null, considering no ICCP");
  } else if (format_record->handleProcs->unlockProc == nullptr) {
    LOG("handleProcs->lockProc is null, considering no ICCP");
  } else if (format_record->iCCprofileData == nullptr) {
    LOG("iCCprofileData is null, considering no ICCP");
  } else {
    Ptr ptr = format_record->handleProcs->lockProc(
        format_record->iCCprofileData, false);
    if (ptr == nullptr) {
      LOG("/!\\ lockProc failed");
      result = vLckdErr;
    } else {
      const int32_t size = format_record->iCCprofileSize;
      if (size <= 0) {
        LOG("iCCprofileSize is " << size << ", considering no ICCP");
      } else {
        LOG("Got " << size << " bytes of color profile.");
        const WebPData view = {(const uint8_t*)ptr, (size_t)size};
        if (!WebPDataCopy(&view, &metadata->chunk)) {
          LOG("/!\\ WebPDataCopy of ICCP chunk (" << size << " bytes) failed.");
          result = memFullErr;
        }
      }
      format_record->handleProcs->unlockProc(format_record->iCCprofileData);
    }
  }
  return result;
}

OSErr GetHostMetadata(FormatRecordPtr format_record,
                      Metadata metadata[Metadata::kNum]) {
  DeallocateMetadata(metadata);  // Get rid of any previous data.

  OSErr result = GetHostProperty(propEXIFData, &metadata[Metadata::kEXIF]);
  if (result != noErr) return result;

  result = GetHostProperty(propXMP, &metadata[Metadata::kXMP]);
  if (result != noErr) return result;

  result = GetHostICCProfile(format_record, &metadata[Metadata::kICCP]);
  if (result != noErr) return result;

  return noErr;
}

bool EncodeMetadata(const WriteConfig& write_config,
                    const Metadata metadata[Metadata::kNum],
                    WebPData* const encoded_data) {
  bool success = true;
  WebPMux* mux = nullptr;  // Only create mux instance when needed.
  const bool keep[Metadata::kNum] = {write_config.keep_exif,
                                     write_config.keep_xmp,
                                     write_config.keep_color_profile};

  for (int i = 0; i < Metadata::kNum; ++i) {
    if (metadata[i].chunk.size > 0 && metadata[i].chunk.bytes != nullptr &&
        keep[i]) {
      if (mux == nullptr) {
        // Copy data into mux object as it will be overwritten.
        mux = WebPMuxCreate(encoded_data, /*copy_data=*/1);
        if (mux == nullptr) {
          LOG("/!\\ WebPMuxCreate failed.");
          success = false;
          break;
        }
      }

      const WebPMuxError mux_error = WebPMuxSetChunk(
          mux, metadata[i].four_cc, &metadata[i].chunk, /*copy_data=*/0);
      if (mux_error != WEBP_MUX_OK) {
        LOG("/!\\ WebPMuxSetChunk(" << metadata[i].four_cc << ") failed ("
                                    << mux_error << ").");
        success = false;
        break;
      } else {
        LOG("Added " << metadata[i].four_cc << " chunk ("
                     << metadata[i].chunk.size << " bytes).");
      }
    }
  }

  if (mux != nullptr) {
    if (success) {
      WebPData mux_data = {nullptr, 0};
      const WebPMuxError mux_error = WebPMuxAssemble(mux, &mux_data);
      if (mux_error != WEBP_MUX_OK) {
        LOG("/!\\ WebPMuxAssemble failed (" << mux_error << ").");
        success = false;
        WebPDataClear(&mux_data);
      } else {
        // Now the muxed data can safely replace the previous one.
        WebPDataClear(encoded_data);
        *encoded_data = mux_data;
        mux_data.bytes = nullptr;  // Do not free memory.
        mux_data.size = 0;
      }
    }
    WebPMuxDelete(mux);
  } else {
    LOG("No metadata written.");
  }
  return success;
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
