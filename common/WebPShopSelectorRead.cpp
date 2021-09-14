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

void SetPlaneColRowBytes(FormatRecordPtr format_record) {
  format_record->loPlane = 0;
  format_record->hiPlane = format_record->planes - 1;

  format_record->planeBytes = format_record->depth / 8;
  format_record->colBytes = format_record->planeBytes * format_record->planes;
  format_record->rowBytes =
      format_record->colBytes * format_record->imageSize32.h;
}

//------------------------------------------------------------------------------

void DoReadPrepare(FormatRecordPtr format_record, Data* const data,
                   int16* const result) {
  format_record->maxData = 0;  // The maximum number of bytes Photoshop can free
                               // up for a plug-in to use.
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
    const WebPData encoded_data = {(uint8_t*)data->file_data, data->file_size};
    if (!DecodeMetadata(encoded_data, data->metadata)) *result = readErr;
  }

  if (*result == noErr) {
    *result = SetHostMetadata(format_record, data->metadata);
  }
  DeallocateMetadata(data->metadata);

  if (*result == noErr) {
    format_record->PluginUsing32BitCoordinates =
        format_record->HostSupports32BitCoordinates;
    format_record->imageMode = plugInModeRGBColor;
    if (!(format_record->hostModes & (1 << format_record->imageMode))) {
      LOG("/!\\ Unsupported plugInModeRGBColor");  // Unlikely.
    }
    format_record->imageSize.h = data->read_config.input.width;
    format_record->imageSize.v = data->read_config.input.height;
    format_record->imageSize32.h = data->read_config.input.width;
    format_record->imageSize32.v = data->read_config.input.height;
    format_record->depth = sizeof(uint8_t) * 8;
    // WebPAnimDecoderNew() only outputs RGBA or BGRA.
    format_record->planes = 4;

    format_record->imageHRes = 72;
    format_record->imageVRes = 72;

    format_record->planeMap[0] = 0;  // RGBA
    format_record->planeMap[1] = 1;
    format_record->planeMap[2] = 2;
    format_record->planeMap[3] = 3;

    SetPlaneColRowBytes(format_record);

    LOG("Host");
    LOG("  transparencyPlane: " << format_record->transparencyPlane);

    format_record->transparencyPlane = 3;
    format_record->transparencyMatting = 0;

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
    LOG("  transparentIndex: " << format_record->transparentIndex);

    LOG("  hostInSecondThread: " << (int)format_record->hostInSecondaryThread);
    LOG("  openAsSmartObject: " << (int)format_record->openAsSmartObject);
  }

  // Going through formatSelectorReadContinue discards the alpha samples for
  // some reason. Treat all images, including still ones, as animations to use
  // formatSelectorReadLayerContinue which supports transparency.
  if (*result == noErr) InitAnimDecoder(format_record, data, result);
  // format_record->layerData = 0;  // Uncomment for formatSelectorReadContinue

  if (*result != noErr) Deallocate(&data->file_data);
}

//------------------------------------------------------------------------------

// FormatLayerSupport in WebPShop.r redirects to DoReadLayerStart() for
// animations. DoReadContinue() is called only for Smart Objects.
void DoReadContinue(FormatRecordPtr format_record, Data* const data,
                    int16* const result) {
  ReadOneFrame(format_record, data, result, /*frame_counter=*/0);
  if (*result != noErr) ReleaseAnimDecoder(format_record, data);

  Deallocate(&data->file_data);
}

//------------------------------------------------------------------------------

void DoReadFinish(FormatRecordPtr format_record, Data* const data,
                  int16* const result) {
  ReleaseAnimDecoder(format_record, data);

  Deallocate(&format_record->data);
  Deallocate(&data->file_data);

  AddComment(format_record, data, result);  // Write a history comment.
}
