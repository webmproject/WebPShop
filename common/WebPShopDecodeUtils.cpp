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

#include "PIProperties.h"
#include "WebPShop.h"
#include "webp/decode.h"
#include "webp/demux.h"

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
                     decoder_config.input.height, /*num_channels=*/4,
                     /*bit_depth=*/8)) {
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

bool DecodeMetadata(const WebPData& encoded_data,
                    Metadata metadata[Metadata::kNum]) {
  DeallocateMetadata(metadata);  // Get rid of any previous data.

  WebPDemuxer* const demux = WebPDemux(&encoded_data);
  if (demux == nullptr) {
    LOG("/!\\ WebPDemux failed.");
    return false;
  }

  bool success = true;
  for (int i = 0; success && i < Metadata::kNum; ++i) {
    WebPChunkIterator iter;
    const int found = WebPDemuxGetChunk(demux, metadata[i].four_cc, 0, &iter);
    if (found != 0 && iter.chunk.bytes != nullptr && iter.chunk.size > 0) {
      if (!WebPDataCopy(&iter.chunk, &metadata[i].chunk)) {
        LOG("/!\\ WebPDataCopy of " << metadata[i].four_cc << " chunk ("
                                    << iter.chunk.size << " bytes) failed.");
        success = false;
      } else {
        LOG("Retrieved " << metadata[i].four_cc << " chunk ("
                         << metadata[i].chunk.size << " bytes).");
      }
    }
    // Only the last chunk of each type is imported.
    WebPDemuxReleaseChunkIterator(&iter);
  }
  WebPDemuxDelete(demux);
  return success;
}

static OSErr SetHostProperty(const Metadata& metadata, PIType key) {
  OSErr result = noErr;
  const WebPData& chunk = metadata.chunk;
  if (chunk.bytes != nullptr && chunk.size > 0) {
    // Code below is inspired from PISetXMP() and PISetEXIFData().
    if (sPSHandle->New == nullptr) {
      LOG("/!\\ sPSHandle->New is null");
      result = errPlugInHostInsufficient;
    } else {
      Handle handle = sPSHandle->New((int32)chunk.size);
      if (handle == nullptr) {
        LOG("/!\\ Could not allocate " << chunk.size
                                       << " bytes with sPSHandle->New()");
        result = memFullErr;
      } else {
        Boolean oldLock = FALSE;
        Ptr ptr = nullptr;
        sPSHandle->SetLock(handle, true, &ptr, &oldLock);
        if (ptr == nullptr) {
          LOG("/!\\ SetLock failed");
          result = vLckdErr;
        } else {
          std::copy(chunk.bytes, chunk.bytes + chunk.size, ptr);
          sPSHandle->SetLock(handle, false, &ptr, &oldLock);
          result = sPSProperty->setPropertyProc(kPhotoshopSignature, key, 0,
                                                NULL, handle);
          if (result != noErr) {
            LOG("/!\\ setPropertyProc failed (" << result << ")");
          } else {
            LOG("Set " << chunk.size << " bytes of " << metadata.four_cc);
          }
        }
        if (result != noErr) sPSHandle->Dispose(handle);
      }
    }
  }
  return result;
}

static OSErr SetHostICCProfile(FormatRecordPtr format_record,
                               const Metadata& metadata) {
  OSErr result = noErr;
  const WebPData& chunk = metadata.chunk;
  if (chunk.bytes != nullptr && chunk.size > 0) {
    if (format_record->handleProcs == nullptr) {
      LOG("handleProcs is null, considering no ICCP");
    } else if (format_record->handleProcs->lockProc == nullptr) {
      LOG("handleProcs->lockProc is null, considering no ICCP");
    } else if (format_record->handleProcs->unlockProc == nullptr) {
      LOG("handleProcs->lockProc is null, considering no ICCP");
    } else {
      const int32 size = (int32)chunk.size;
      Handle handle = sPSHandle->New(size);
      if (handle == nullptr) {
        LOG("/!\\ Could not allocate " << size
                                       << " bytes with sPSHandle->New()");
        result = memFullErr;
      } else {
        Ptr ptr = format_record->handleProcs->lockProc(handle, false);
        if (ptr == nullptr) {
          LOG("/!\\ lockProc failed");
          result = vLckdErr;
          sPSHandle->Dispose(handle);
        } else {
          std::copy(chunk.bytes, chunk.bytes + chunk.size, ptr);
          format_record->handleProcs->unlockProc(handle);

          format_record->iCCprofileData = handle;
          format_record->iCCprofileSize = size;
          LOG("Set " << size << " bytes of color profile.");
        }
      }
    }
  }
  return result;
}

OSErr SetHostMetadata(FormatRecordPtr format_record,
                      const Metadata metadata[Metadata::kNum]) {
  OSErr result = SetHostProperty(metadata[Metadata::kEXIF], propEXIFData);
  if (result != noErr) return result;

  result = SetHostProperty(metadata[Metadata::kXMP], propXMP);
  if (result != noErr) return result;

  result = SetHostICCProfile(format_record, metadata[Metadata::kICCP]);
  if (result != noErr) return result;

  return noErr;
}
