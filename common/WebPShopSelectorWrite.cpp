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

void DoWritePrepare(FormatRecordPtr format_record, Data* const data,
                    int16* const result) {
  format_record->maxData = 0;
  LoadPOSIXConfig(format_record, result);
}

//------------------------------------------------------------------------------

void DoWriteStart(FormatRecordPtr format_record, Data* const data,
                  int16* const result) {
  format_record->planeMap[0] = 2;  // BGRA
  format_record->planeMap[1] = 1;
  format_record->planeMap[2] = 0;
  format_record->planeMap[3] = 3;

  SetPlaneColRowBytes(format_record);

  LOG("layerData: " << format_record->layerData);
  LOG("loPlane: " << format_record->loPlane);
  LOG("hiPlane: " << format_record->hiPlane);

  LOG("planeBytes: " << format_record->planeBytes);
  LOG("colBytes: " << format_record->colBytes);
  LOG("rowBytes: " << format_record->rowBytes);

  if (data->encoded_data.bytes == nullptr) {
    // We could use format_record->data and DoWrite*() but we'd have to
    // handle RGB. Copy*() always return RGBA, which is needed for WebPEncode().
    if (data->write_config.animation) {
      std::vector<FrameMemoryDesc> original_frames;
      CopyAllLayers(format_record, data, result, &original_frames);

      if (*result == noErr &&
          (!EncodeAllFrames(original_frames, data->write_config,
                            &data->encoded_data) ||
           data->encoded_data.bytes == nullptr ||
           data->encoded_data.size == 0)) {
        *result = writErr;
      }
      ClearFrameVector(&original_frames);
    } else {  // !data->write_config.animation
      ImageMemoryDesc image;
      CopyWholeCanvas(format_record, data, result, &image);

      if (*result == noErr &&
          (!EncodeOneImage(image, data->write_config, &data->encoded_data) ||
           data->encoded_data.bytes == nullptr ||
           data->encoded_data.size == 0)) {
        *result = writErr;
      }
      DeallocateImage(&image);
    }
    if (*result == noErr && !EncodeMetadata(data->write_config, data->metadata,
                                            &data->encoded_data)) {
      *result = writErr;
    }
  }
  RequestWholeCanvas(format_record, result);  // Prevent inf loop.

  if (*result != noErr) Deallocate(&format_record->data);
}

//------------------------------------------------------------------------------

void DoWriteContinue(FormatRecordPtr format_record, Data* const data,
                     int16* const result) {
  if (*result == noErr) {
    WriteToFile(data->encoded_data, format_record, result);
  }

  WebPDataClear(&data->encoded_data);
  DeallocateMetadata(data->metadata);
  Deallocate(&format_record->data);
}

//------------------------------------------------------------------------------

void DoWriteFinish(FormatRecordPtr format_record, Data* const data,
                   int16* const result) {
  // Should be empty. Just in case.
  WebPDataClear(&data->encoded_data);
  DeallocateMetadata(data->metadata);
  Deallocate(&format_record->data);

  if (*result == noErr) {
    SaveWriteConfig(format_record, data->write_config, result);
  }
}
