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

#include <vector>

#include "PIFormat.h"
#include "WebPShop.h"

//------------------------------------------------------------------------------

static int CountChannels(const ReadChannelDesc* rgb_channels,
                         const ReadChannelDesc* a_channels) {
  const ReadChannelDesc* channel = rgb_channels;
  int channel_index = 0;  // To prevent hypothetical infinite loops.
  int r = 0, g = 0, b = 0, a = 0;

  while (channel != nullptr && channel_index < MAX_NUM_BROWSED_CHANNELS) {
    if (channel->channelType == ctRed) {
      r = 1;
    } else if (channel->channelType == ctGreen) {
      g = 1;
    } else if (channel->channelType == ctBlue) {
      b = 1;
    }
    ++channel_index;
    channel = channel->next;
  }

  channel = a_channels;
  channel_index = 0;

  while (channel != nullptr && channel_index < MAX_NUM_BROWSED_CHANNELS) {
    if (channel->channelType == ctLayerMask ||
        channel->channelType == ctTransparency) {
      a = 1;
    }
    ++channel_index;
    channel = channel->next;
  }

  return r + g + b + a;
}

//------------------------------------------------------------------------------

static void CopyChannel(Data* const data, int16* const result,
                        const ReadChannelDesc* const channel,
                        ImageMemoryDesc* const destination) {
  if (channel->port == nullptr) {
    LOG("/!\\ The channel has no port.");
    *result = writErr;
    return;
  }

  // BGRA order.
  switch (channel->channelType) {
    case ctRed: {
      destination->pixels.bitOffset = channel->depth * 2;
      break;
    }
    case ctGreen: {
      destination->pixels.bitOffset = channel->depth * 1;
      break;
    }
    case ctBlue: {
      destination->pixels.bitOffset = channel->depth * 0;
      break;
    }
    case ctLayerMask:
    case ctTransparency: {
      destination->pixels.bitOffset = channel->depth * 3;
      break;
    }
    default: {
      LOG("Discarding channel.");
      return;
    }
  }

  Boolean canRead = false;
  if (data->sPSChannelPortsSuite == nullptr ||
      data->sPSChannelPortsSuite->CanRead == nullptr ||
      data->sPSChannelPortsSuite->CanRead(channel->port, &canRead)) {
    LOG("/!\\ Unable to read from channel's port.");
    *result = errPlugInHostInsufficient;
    return;
  }

  VRect read_rect;
  read_rect.left = 0;
  read_rect.right = destination->width;
  read_rect.top = 0;
  read_rect.bottom = destination->height;

  if (data->sPSChannelPortsSuite->ReadPixelsFromLevel(
          channel->port, 0, &read_rect, &destination->pixels)) {
    LOG("/!\\ ReadPixelsFromLevel() failed.");
    LOG("Channel " << channel->channelType);
    LOG("  depth: " << channel->depth << " / " << destination->pixels.depth);
    *result = errPlugInHostInsufficient;
  } else if (GetWidth(read_rect) != destination->width ||
             GetHeight(read_rect) != destination->height) {
    LOG("/!\\ Unable to read whole area.");
    LOG("Read:");
    LOG("  left    " << read_rect.left << ", right   " << read_rect.right);
    LOG("  top     " << read_rect.top << ", bottom  " << read_rect.bottom);
    LOG("Expected:");
    LOG("  width " << destination->width << ", height " << destination->height);
    *result = writErr;
  }
}

static void CopyChannels(Data* const data, int16* const result,
                         const ReadChannelDesc* rgb_channels,
                         const ReadChannelDesc* a_channels, int32 canvas_width,
                         int32 canvas_height, int32 bit_depth,
                         ImageMemoryDesc* const destination) {
  if (rgb_channels == nullptr || destination == nullptr) {
    LOG("/!\\ Source or destination is null.");
    *result = writErr;
    return;
  }

  const int num_channels = CountChannels(rgb_channels, a_channels);
  if (num_channels < 3 || num_channels > 4) {
    LOG("/!\\ Unhandled number of channels: " << num_channels);
    *result = writErr;
    return;
  }

  // The pixels can only be extracted with their original bit depth.
  // Four 8-bit channels are needed by WebPEncode() so either allocate them
  // directly or just what is necessary and downscale afterwards.
  // TODO: Modify CopyChannel() to read line by line to avoid allocating
  //       the whole canvas twice.
  ImageMemoryDesc destination_16b_or_32b;
  ImageMemoryDesc* const tmp_dst =
      (bit_depth == 8) ? destination : &destination_16b_or_32b;
  if (!AllocateImage(tmp_dst, canvas_width, canvas_height,
                     (bit_depth == 8) ? 4 : num_channels, bit_depth)) {
    LOG("/!\\ AllocateImage() failed.");
    *result = memFullErr;
    return;
  }

  const ReadChannelDesc* channel = rgb_channels;
  size_t channel_index = 0;

  while (channel != nullptr && channel_index < MAX_NUM_BROWSED_CHANNELS &&
         *result == noErr) {
    CopyChannel(data, result, channel, tmp_dst);

    ++channel_index;
    channel = channel->next;
  }

  channel = a_channels;
  channel_index = 0;

  while (channel != nullptr && channel_index < MAX_NUM_BROWSED_CHANNELS &&
         *result == noErr) {
    CopyChannel(data, result, channel, tmp_dst);

    ++channel_index;
    channel = channel->next;
  }

  if (num_channels == 3 && tmp_dst->pixels.depth == 8) {
    for (size_t y = 0; y < tmp_dst->height; ++y) {
      uint8_t* dst_data = reinterpret_cast<uint8_t*>(tmp_dst->pixels.data) +
                          y * (tmp_dst->pixels.rowBits / 8) +
                          /*channel=*/3 * (tmp_dst->pixels.depth / 8);
      for (size_t x = 0; x < tmp_dst->width;
           ++x, dst_data += tmp_dst->pixels.colBits / 8) {
        *dst_data = 255;  // Missing opaque alpha channel.
      }
    }
  }

  if (tmp_dst != destination &&
      !To8bit(destination_16b_or_32b,
              /*add_alpha=*/(destination_16b_or_32b.num_channels < 4),
              destination)) {
    *result = writErr;
  }

  DeallocateImage(&destination_16b_or_32b);
}

//------------------------------------------------------------------------------

static bool GetDocumentDimensions(FormatRecordPtr format_record,
                                  int16* const result,
                                  int32* const canvas_width,
                                  int32* const canvas_height,
                                  int32* const bit_depth) {
  if (format_record == nullptr || format_record->documentInfo == nullptr ||
      canvas_width == nullptr || canvas_height == nullptr) {
    LOG("/!\\ Unable to access document info or output.");
    *result = writErr;
    return false;
  }
  *canvas_width = GetWidth(format_record->documentInfo->bounds);
  *canvas_height = GetHeight(format_record->documentInfo->bounds);
  *bit_depth = format_record->documentInfo->depth;
  if (*canvas_width <= 0 || *canvas_height <= 0) {
    LOG("/!\\ Width or height <= 0.");
    *result = writErr;
    return false;
  }
  return true;
}

void CopyWholeCanvas(FormatRecordPtr format_record, Data* const data,
                     int16* const result, ImageMemoryDesc* const destination) {
  START_TIMER(CopyWholeCanvas);

  int32 canvas_width, canvas_height, bit_depth;
  if (!GetDocumentDimensions(format_record, result, &canvas_width,
                             &canvas_height, &bit_depth)) {
    return;
  }

  CopyChannels(data, result,
               format_record->documentInfo->mergedCompositeChannels,
               format_record->documentInfo->mergedTransparency, canvas_width,
               canvas_height, bit_depth, destination);
  LOG("Copied " << destination->num_channels << " channels.");

  STOP_TIMER(CopyWholeCanvas);
}

void CopyAllLayers(FormatRecordPtr format_record, Data* const data,
                   int16* const result,
                   std::vector<FrameMemoryDesc>* const destination) {
  START_TIMER(CopyAllLayers);

  if (destination == nullptr) {
    LOG("/!\\ Destination is null.");
    *result = writErr;
    return;
  }

  int32 canvas_width, canvas_height, bit_depth;
  if (!GetDocumentDimensions(format_record, result, &canvas_width,
                             &canvas_height, &bit_depth)) {
    return;
  }

  const size_t expected_num_layers =
      (size_t)format_record->documentInfo->layerCount;
  if (expected_num_layers < 1 ||
      expected_num_layers >= MAX_NUM_BROWSED_LAYERS) {
    LOG("/!\\ Invalid number of layers (" << expected_num_layers << ").");
    *result = writErr;
    return;
  }

  ResizeFrameVector(destination, expected_num_layers);

  const ReadLayerDesc* layer_desc =
      format_record->documentInfo->layersDescriptor;
  size_t layer_count = 0;
  while (*result == noErr && layer_desc != nullptr &&
         layer_count < expected_num_layers) {
    int frame_duration;
    if (!TryExtractDuration(layer_desc->unicodeName, &frame_duration)) {
      LOG("/!\\ Can't extract duration from layer name.");
      *result = writErr;
    } else {
      FrameMemoryDesc& frame = (*destination)[layer_count];
      frame.duration_ms = frame_duration;

      CopyChannels(data, result, layer_desc->compositeChannelsList,
                   layer_desc->transparency, canvas_width, canvas_height,
                   bit_depth, &frame.image);
    }
    layer_desc = layer_desc->next;
    ++layer_count;
  }
  LOG("Copied " << layer_count << " / " << expected_num_layers << " layers.");
  if (layer_count != expected_num_layers) {
    LOG("/!\\ Missing layers.");
    *result = writErr;
  }

  STOP_TIMER(CopyAllLayers);
}

//------------------------------------------------------------------------------

void RequestWholeCanvas(FormatRecordPtr format_record, int16* const result) {
  format_record->theRect32.top = 0;
  format_record->theRect32.bottom = format_record->imageSize32.v;
  format_record->theRect32.left = 0;
  format_record->theRect32.right = format_record->imageSize32.h;

  const size_t data_size =
      (size_t)(format_record->rowBytes * format_record->imageSize32.v);

  LOG("Allocate " << data_size << " bytes.");
  Allocate(data_size, &format_record->data, result);
  // Image will be copied in format_record->data by Photoshop.
}
