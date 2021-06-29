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

//------------------------------------------------------------------------------

void DoEstimatePrepare(FormatRecordPtr format_record, Data* const data,
                       int16* const result) {
  format_record->maxData = 0;  // The maximum number of bytes Photoshop can free
                               // up for a plug-in to use.
}

//------------------------------------------------------------------------------

void DoEstimateStart(FormatRecordPtr format_record, Data* const data,
                     int16* const result) {
  if (data->encoded_data.bytes != nullptr) {
    // Output size is already known.
    format_record->minDataBytes = (int32)data->encoded_data.size;
    format_record->maxDataBytes = (int32)data->encoded_data.size;
  } else {
    // Best and worst case estimation.
    int32 width = (format_record->HostSupports32BitCoordinates
                       ? format_record->imageSize32.h
                       : format_record->imageSize.h);
    int32 height = (format_record->HostSupports32BitCoordinates
                        ? format_record->imageSize32.v
                        : format_record->imageSize.v);

    format_record->minDataBytes = 20;
    format_record->maxDataBytes =
        width * height * format_record->planes * format_record->planeBytes;

    if (data->write_config.animation) {
      format_record->maxDataBytes *= format_record->layerData;
    }

    // Maximum overhead compared to raw.
    format_record->maxDataBytes += 200;
  }

  format_record->data = nullptr;
}

//------------------------------------------------------------------------------

void DoEstimateContinue(FormatRecordPtr format_record, Data* const data,
                        int16* const result) {}

//------------------------------------------------------------------------------

void DoEstimateFinish(FormatRecordPtr format_record, Data* const data,
                      int16* const result) {}
