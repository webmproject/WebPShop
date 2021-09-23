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

#include <cmath>

//------------------------------------------------------------------------------

bool AllocateImage(ImageMemoryDesc* const image, int32 width, int32 height,
                   int num_channels, int32 bit_depth) {
  if (image == nullptr) {
    LOG("/!\\ Source is null.");
    return false;
  }
  int16 result = noErr;
  if (image->pixels.data == nullptr || (image->width != width) ||
      (image->height != height) || (image->num_channels != num_channels)) {
    DeallocateImage(image);
    image->width = width;
    image->height = height;
    image->num_channels = num_channels;
    image->mode = plugInModeRGBColor;
    image->pixels.depth = bit_depth;
    image->pixels.colBits = image->pixels.depth * image->num_channels;
    image->pixels.rowBits = image->pixels.colBits * width;
    const size_t image_data_size = (size_t)(image->pixels.rowBits / 8) * height;
    Allocate(image_data_size, &image->pixels.data, &result);
  }
  return result == noErr;
}

void DeallocateImage(ImageMemoryDesc* const image) {
  if (image == nullptr) {
    LOG("/!\\ Source is null.");
    return;
  }
  Deallocate(&image->pixels.data);
}

void DeallocateMetadata(Metadata metadata[Metadata::kNum]) {
  for (int i = 0; i < Metadata::kNum; ++i) {
    WebPDataClear(&metadata[i].chunk);
    metadata[i].chunk.size = 0;  // Extra safety in case NULL but size>0.
  }
}

//------------------------------------------------------------------------------

void ResizeFrameVector(std::vector<FrameMemoryDesc>* const frames,
                       size_t size) {
  if (frames == nullptr) {
    LOG("/!\\ Source is null.");
    return;
  }
  for (size_t i = size; i < frames->size(); ++i) {
    DeallocateImage(&(*frames)[i].image);
  }
  frames->resize(size);
}
void ClearFrameVector(std::vector<FrameMemoryDesc>* const frames) {
  ResizeFrameVector(frames, 0);
}

//------------------------------------------------------------------------------

bool Scale(const ImageMemoryDesc& src, ImageMemoryDesc* const dst,
           size_t dst_width, size_t dst_height) {
  if (src.pixels.data == nullptr || src.width < 1 || src.height < 1 ||
      (src.pixels.depth % 8) != 0 || dst == nullptr || &src == dst) {
    LOG("/!\\ Invalid source or destination.");
    return false;
  }
  if (!AllocateImage(dst, (int32)dst_width, (int32)dst_height,
                     src.num_channels, src.pixels.depth)) {
    LOG("/!\\ AllocateImage failed.");
    return false;
  }
  dst->mode = src.mode;

  for (size_t dst_x = 0; dst_x < dst_width; ++dst_x) {
    const size_t src_x = dst_x * src.width / dst_width;
    for (size_t dst_y = 0; dst_y < dst_height; ++dst_y) {
      const size_t src_y = dst_y * src.height / dst_height;
      for (int channel = 0; channel < dst->num_channels; ++channel) {
        const uint8_t* src_data =
            reinterpret_cast<const uint8_t*>(src.pixels.data) +
            src_x * (src.pixels.colBits / 8) +
            src_y * (src.pixels.rowBits / 8) + channel * (src.pixels.depth / 8);
        uint8_t* dst_data = reinterpret_cast<uint8_t*>(dst->pixels.data) +
                            dst_x * (dst->pixels.colBits / 8) +
                            dst_y * (dst->pixels.rowBits / 8) +
                            channel * (dst->pixels.depth / 8);
        // No interpolation.
        std::copy(src_data, src_data + (dst->pixels.depth / 8), dst_data);
      }
    }
  }
  return true;
}

bool Crop(const ImageMemoryDesc& src, ImageMemoryDesc* const dst,
          size_t crop_width, size_t crop_height, size_t crop_left,
          size_t crop_top) {
  if (src.pixels.data == nullptr || src.width < 1 || src.height < 1 ||
      (src.pixels.depth % 8) != 0 || dst == nullptr || &src == dst) {
    LOG("/!\\ Invalid source or destination.");
    return false;
  }
  if (crop_width < 1 || crop_height < 1 ||
      crop_left + crop_width > (size_t)src.width ||
      crop_top + crop_height > (size_t)src.height) {
    LOG("/!\\ Invalid input.");
    return false;
  }
  if (!AllocateImage(dst, (int32)crop_width, (int32)crop_height,
                     src.num_channels, src.pixels.depth)) {
    LOG("/!\\ AllocateImage failed.");
    return false;
  }
  dst->mode = src.mode;

  for (size_t dst_x = 0; dst_x < crop_width; ++dst_x) {
    const size_t src_x = crop_left + dst_x;
    for (size_t dst_y = 0; dst_y < crop_height; ++dst_y) {
      const size_t src_y = crop_top + dst_y;
      for (int channel = 0; channel < dst->num_channels; ++channel) {
        const uint8_t* src_data =
            reinterpret_cast<const uint8_t*>(src.pixels.data) +
            src_x * (src.pixels.colBits / 8) +
            src_y * (src.pixels.rowBits / 8) + channel * (src.pixels.depth / 8);
        uint8_t* dst_data = reinterpret_cast<uint8_t*>(dst->pixels.data) +
                            dst_x * (dst->pixels.colBits / 8) +
                            dst_y * (dst->pixels.rowBits / 8) +
                            channel * (dst->pixels.depth / 8);
        std::copy(src_data, src_data + (dst->pixels.depth / 8), dst_data);
      }
    }
  }
  return true;
}

bool To8bit(const ImageMemoryDesc& src, bool add_alpha,
            ImageMemoryDesc* const dst) {
  if (src.width < 1 || src.height < 1 || dst == nullptr ||
      (add_alpha && src.num_channels != 3) || &src == dst) {
    LOG("/!\\ Invalid source or destination.");
    return false;
  }
  if (src.pixels.depth != 8 && src.pixels.depth != 16 &&
      src.pixels.depth != 32) {
    LOG("/!\\ Unsupported source depth " << src.pixels.depth << ".");
    return false;
  }

  if (!AllocateImage(dst, src.width, src.height,
                     src.num_channels + (add_alpha ? 1 : 0), /*depth=*/8)) {
    LOG("/!\\ AllocateImage failed.");
    return false;
  }

  // Not much documentation was found besides SDK FAQ for 16b but it seems that:
  // Photoshop Image Mode | 8 Bits/Channel | 16 Bits/Channel | 32 Bits/Channel
  // ---------------------|----------------|-----------------|----------------
  //       RGB Color      |     [0:255]    |    [0:32768]    |    [0.f:1.f]

  for (size_t y = 0; y < src.height; ++y) {
    for (size_t x = 0; x < src.width; ++x) {
      for (int channel = 0; channel < dst->num_channels; ++channel) {
        uint8_t* dst_data = reinterpret_cast<uint8_t*>(dst->pixels.data) +
                            x * (dst->pixels.colBits / 8) +
                            y * (dst->pixels.rowBits / 8) +
                            channel * (dst->pixels.depth / 8);
        if (channel >= src.num_channels) {
          *dst_data = 255;  // add_alpha
          continue;
        }

        const uint8_t* src_data =
            reinterpret_cast<const uint8_t*>(src.pixels.data) +
            x * (src.pixels.colBits / 8) + y * (src.pixels.rowBits / 8) +
            channel * (src.pixels.depth / 8);
        if (src.pixels.depth == 8) {
          *dst_data = *src_data;
        } else if (src.pixels.depth == 16) {
          uint32_t value = *reinterpret_cast<const uint16_t*>(src_data);
          value >>= 7;
          *dst_data = (value >= 255) ? 255 : static_cast<uint8_t>(value);
        } else {
          float value = *reinterpret_cast<const float*>(src_data) * 255.f;
          *dst_data =
              (value >= 255.f) ? 255 : static_cast<uint8_t>(std::lrint(value));
        }
      }
    }
  }
  return true;
}
