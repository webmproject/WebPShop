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

//------------------------------------------------------------------------------

bool AllocateImage(ImageMemoryDesc* const image, int32 width, int32 height,
                   int num_channels) {
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
    image->pixels.depth = (int32)sizeof(uint8_t) * 8;
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
      dst == nullptr) {
    LOG("/!\\ Source or destination is null.");
    return false;
  }
  if (!AllocateImage(dst, (int32)dst_width, (int32)dst_height,
                     src.num_channels)) {
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
            (const uint8_t*)src.pixels.data + src_x * (src.pixels.colBits / 8) +
            src_y * (src.pixels.rowBits / 8) + channel * (src.pixels.depth / 8);
        uint8_t* dst_data = (uint8_t*)dst->pixels.data +
                            dst_x * (dst->pixels.colBits / 8) +
                            dst_y * (dst->pixels.rowBits / 8) +
                            channel * (dst->pixels.depth / 8);
        *dst_data = *src_data;  // No interpolation.
      }
    }
  }
  return true;
}

bool Crop(const ImageMemoryDesc& src, ImageMemoryDesc* const dst,
          size_t crop_width, size_t crop_height, size_t crop_left,
          size_t crop_top) {
  if (src.pixels.data == nullptr || src.width < 1 || src.height < 1 ||
      dst == nullptr) {
    LOG("/!\\ Source or destination is null.");
    return false;
  }
  if (crop_width < 1 || crop_height < 1 ||
      crop_left + crop_width > (size_t)src.width ||
      crop_top + crop_height > (size_t)src.height) {
    LOG("/!\\ Invalid input.");
    return false;
  }
  if (!AllocateImage(dst, (int32)crop_width, (int32)crop_height,
                     src.num_channels)) {
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
            (const uint8_t*)src.pixels.data + src_x * (src.pixels.colBits / 8) +
            src_y * (src.pixels.rowBits / 8) + channel * (src.pixels.depth / 8);
        uint8_t* dst_data = (uint8_t*)dst->pixels.data +
                            dst_x * (dst->pixels.colBits / 8) +
                            dst_y * (dst->pixels.rowBits / 8) +
                            channel * (dst->pixels.depth / 8);
        *dst_data = *src_data;
      }
    }
  }
  return true;
}
