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

#include "PITypes.h"
#include "WebPShop.h"

int32 GetWidth(const VRect& rect) { return rect.right - rect.left; }
int32 GetHeight(const VRect& rect) { return rect.bottom - rect.top; }

//------------------------------------------------------------------------------

bool ScaleToFit(int32* const width, int32* const height, int32 max_width,
                int32 max_height) {
  if (width == nullptr || height == nullptr) {
    LOG("/!\\ Width or height is null.");
    return false;
  }
  if (*width <= 0 || *height <= 0 || max_width <= 0 || max_height <= 0) {
    LOG("/!\\ Invalid input.");
    return false;
  }
  if (*width > max_width || *height > max_height) {
    const int32 original_width = *width;
    const int32 original_height = *height;
    // Try resizing based on width first to see if it fits.
    *width = max_width;
    *height = (original_height * max_width) / original_width;
    *height = (*height < 1) ? 1 : *height;
    // If it doesn't, resize based on height.
    if (*height > max_height) {
      *width = (original_width * max_height) / original_height;
      *width = (*width < 1) ? 1 : *width;
      *height = max_height;
    }
    return true;
  }
  return false;
}

bool CropToFit(int32* const width, int32* const height, int32 left, int32 top,
               int32 max_width, int32 max_height) {
  if (width == nullptr || height == nullptr) {
    LOG("/!\\ Width or height is null.");
    return false;
  }
  if (*width <= 0 || *height <= 0 || left < 0 || top < 0 || left >= *width ||
      top >= *height || max_width <= 0 || max_height <= 0) {
    LOG("/!\\ Invalid input.");
    return false;
  }
  if (left == 0 && top == 0 && *width <= max_width && *height <= max_height) {
    return false;
  }
  *width -= left;
  *height -= top;
  if (*width > max_width) *width = max_width;
  if (*height > max_height) *height = max_height;
  return true;
}

VRect ScaleRectFromAreaToArea(const VRect& src, int32 src_area_width,
                              int32 src_area_height, int32 dst_area_width,
                              int32 dst_area_height) {
  VRect dst;
  dst.left = (src.left * dst_area_width) / src_area_width;
  dst.right = (src.right * dst_area_width) / src_area_width;
  dst.top = (src.top * dst_area_height) / src_area_height;
  dst.bottom = (src.bottom * dst_area_height) / src_area_height;
  return dst;
}
