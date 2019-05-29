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

#include "PIFormat.h"
#include "WebPShop.h"
#include "WebPShopSelector.h"
#include "webp/decode.h"

void ReadOneImage(FormatRecordPtr format_record, Data* const data,
                  int16* const result) {
  START_TIMER(ReadOneImage);

  format_record->theRect32.top = 0;
  format_record->theRect32.bottom = format_record->imageSize32.v;
  format_record->theRect32.left = 0;
  format_record->theRect32.right = format_record->imageSize32.h;

  if (format_record->planes == 3) {
    data->read_config.output.colorspace = MODE_BGR;
  } else {
    data->read_config.output.colorspace = MODE_BGRA;
  }
  data->read_config.output.u.RGBA.stride = format_record->rowBytes;
  data->read_config.output.u.RGBA.size =
      (size_t)(format_record->rowBytes * format_record->imageSize32.v);
  data->read_config.output.is_external_memory = 1;

  Allocate(data->read_config.output.u.RGBA.size, &format_record->data, result);
  data->read_config.output.u.RGBA.rgba = (uint8_t*)format_record->data;

  LOG("Uncompressed size: " << data->read_config.output.u.RGBA.size << " B");

  if (*result == noErr) {
    VP8StatusCode status = WebPDecode((uint8_t*)data->file_data,
                                      data->file_size, &data->read_config);
    LOG("WebPDecode (" << status << ")");
    if (status != VP8_STATUS_OK) {
      *result = readErr;
    }
  }

  data->read_config.output.u.RGBA.rgba = nullptr;

  if (*result == noErr) *result = format_record->advanceState();
  format_record->progressProc(1, 1);

  Deallocate(&format_record->data);

  STOP_TIMER(ReadOneImage);
}

//------------------------------------------------------------------------------

void DoReadPrepare(FormatRecordPtr format_record, Data* const data,
                   int16* const result) {
  format_record->maxData = 0;
  LoadPOSIXConfig(format_record, result);
}

//------------------------------------------------------------------------------

void DoReadStart(FormatRecordPtr format_record, Data* const data,
                 int16* const result) {
  ReadAndCheckHeader(format_record, result, &data->file_size);
  if (*result != noErr) return;

  LOG("Allocate and read " << data->file_size << " bytes.");
  AllocateAndRead(data->file_size, &data->file_data, format_record, result);
  if (*result != noErr) return;

  if (*result == noErr && !WebPInitDecoderConfig(&data->read_config)) {
    LOG("/!\\ WebPInitDecoderConfig() failed.");
    *result = readErr;
  }

  if (*result == noErr &&
      WebPGetFeatures((uint8_t*)data->file_data, data->file_size,
                      &data->read_config.input) != VP8_STATUS_OK) {
    LOG("/!\\ WebPGetFeatures() failed.");
    *result = readErr;
  }

  if (*result == noErr) {
    format_record->imageMode = plugInModeRGBColor;
    format_record->imageSize32.h = data->read_config.input.width;
    format_record->imageSize32.v = data->read_config.input.height;
    format_record->depth = sizeof(uint8_t) * 8;
    format_record->planes = data->read_config.input.has_alpha ? 4 : 3;

    format_record->imageHRes = 72;
    format_record->imageVRes = 72;

    format_record->loPlane = 0;
    format_record->hiPlane = format_record->planes - 1;
    format_record->planeMap[0] = 2;  // BGRA
    format_record->planeMap[1] = 1;
    format_record->planeMap[2] = 0;
    format_record->planeMap[3] = 3;

    format_record->planeBytes = format_record->depth / 8;
    format_record->colBytes = format_record->planeBytes * format_record->planes;
    format_record->rowBytes =
        (int16)format_record->colBytes * format_record->imageSize32.h;

    if (data->read_config.input.has_alpha) {
      format_record->transparencyPlane = 3;
      format_record->transparencyMatting = 0;
    }

    LOG("Params (" << data->read_config.input.width << "x"
                   << data->read_config.input.height << "px)");
    LOG("  imageMode: " << format_record->imageMode);
    LOG("  depth: " << format_record->depth);
    LOG("  planes: " << format_record->planes);
    LOG("  layerData: " << format_record->layerData);

    LOG("  loPlane: " << format_record->loPlane);
    LOG("  hiPlane: " << format_record->hiPlane);

    LOG("  planeBytes: " << format_record->planeBytes);
    LOG("  colBytes: " << format_record->colBytes);
    LOG("  rowBytes: " << format_record->rowBytes);

    LOG("  transparencyPlane: " << format_record->transparencyPlane);
    LOG("  transparencyMatting: " << format_record->transparencyMatting);
  }

  if (data->read_config.input.has_animation) {
    // Switch to animation.
    if (*result == noErr) InitAnimDecoder(format_record, data, result);
  } else {
    // We could force DoReadContinue() but alpha won't work.
    // format_record->layerData = 0;
    // Instead we call DoReadStartLayer() with one frame.
    format_record->layerData = 1;
  }

  if (*result != noErr) Deallocate(&data->file_data);
}

//------------------------------------------------------------------------------

// FormatLayerSupport in WebPShop.r redirects to DoReadLayerStart().
// DoReadContinue() should not be called, but it's here just in case.
void DoReadContinue(FormatRecordPtr format_record, Data* const data,
                    int16* const result) {
  ReadOneImage(format_record, data, result);

  Deallocate(&data->file_data);
}

//------------------------------------------------------------------------------

void DoReadFinish(FormatRecordPtr format_record, Data* const data,
                  int16* const result) {
  if (format_record->layerData > 0) {
    ReleaseAnimDecoder(format_record, data);
  }

  Deallocate(&format_record->data);
  Deallocate(&data->file_data);

  AddComment(format_record, data, result);  // Write a history comment.
}
