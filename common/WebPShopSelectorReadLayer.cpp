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
#include "webp/demux.h"

void InitAnimDecoder(FormatRecordPtr format_record, Data* const data,
                     int16* const result) {
  WebPData webp_data;
  webp_data.bytes = (uint8_t*)data->file_data;
  webp_data.size = data->file_size;

  data->anim_decoder = WebPAnimDecoderNew(&webp_data, nullptr);
  if (data->anim_decoder == nullptr) {
    LOG("/!\\ WebPAnimDecoderNew() failed.");
    *result = readErr;
    return;
  }

  WebPAnimInfo info;
  if (!WebPAnimDecoderGetInfo(data->anim_decoder, &info)) {
    LOG("/!\\ WebPAnimDecoderGetInfo() failed.");
    *result = readErr;
  } else if (info.frame_count == 0) {
    LOG("/!\\ There is no frame to decode.");
    *result = readErr;
  } else {
    format_record->layerData = info.frame_count;
    data->last_frame_timestamp = 0;
    LOG("Will decode " << format_record->layerData << " frames.");
  }

  if (*result != noErr) ReleaseAnimDecoder(format_record, data);
}

void ReadOneFrame(FormatRecordPtr format_record, Data* const data,
                  int16* const result, int frame_counter) {
  START_TIMER(ReadOneFrame);
  if (WebPAnimDecoderHasMoreFrames(data->anim_decoder)) {
    uint8_t* buf;
    int timestamp;
    if (!WebPAnimDecoderGetNext(data->anim_decoder, &buf, &timestamp)) {
      LOG("/!\\ WebPAnimDecoderGetNext() failed");
      *result = readErr;
      return;
    }
    format_record->theRect.top = 0;
    format_record->theRect.bottom = format_record->imageSize.v;
    format_record->theRect.left = 0;
    format_record->theRect.right = format_record->imageSize.h;
    format_record->theRect32.top = 0;
    format_record->theRect32.bottom = format_record->imageSize32.v;
    format_record->theRect32.left = 0;
    format_record->theRect32.right = format_record->imageSize32.h;
    // Leave blendMode and opacity as is, it works.

    SetPlaneColRowBytes(format_record);

    format_record->data = buf;

    *result = format_record->advanceState();
    format_record->progressProc(1, 1);
    format_record->data = nullptr;

    // Only rename the layer if it is an animated WebP.
    if (data->read_config.input.has_animation) {
      const int frame_duration = timestamp - data->last_frame_timestamp;
      const size_t layer_name_buffer_length =
          sizeof(data->layer_name_buffer) / sizeof(data->layer_name_buffer[0]);
      PrintDuration(frame_counter, frame_duration, data->layer_name_buffer,
                    layer_name_buffer_length);
      format_record->layerName = data->layer_name_buffer;
      data->last_frame_timestamp = timestamp;
    }
  } else {
    format_record->data = nullptr;
  }
  STOP_TIMER(ReadOneFrame);
}

void ReleaseAnimDecoder(FormatRecordPtr format_record, Data* const data) {
  WebPAnimDecoderDelete(data->anim_decoder);
  data->anim_decoder = nullptr;
  format_record->data = nullptr;
  Deallocate(&data->file_data);
}

//------------------------------------------------------------------------------

void DoReadLayerStart(FormatRecordPtr format_record, Data* const data,
                      int16* const result) {
  if (*result != noErr) ReleaseAnimDecoder(format_record, data);
}

//------------------------------------------------------------------------------

void DoReadLayerContinue(FormatRecordPtr format_record, Data* const data,
                         int16* const result) {
  ReadOneFrame(format_record, data, result, format_record->layerData);

  if (*result != noErr) ReleaseAnimDecoder(format_record, data);
}

//------------------------------------------------------------------------------

void DoReadLayerFinish(FormatRecordPtr format_record, Data* const data,
                       int16* const result) {
  if (*result != noErr) ReleaseAnimDecoder(format_record, data);
}
