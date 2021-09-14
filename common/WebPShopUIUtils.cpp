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

#include <string>

#include "PIFormat.h"
#include "WebPShop.h"

//------------------------------------------------------------------------------

VRect NullRect() {
  VRect rect;
  rect.left = rect.right = rect.top = rect.bottom = 0;
  return rect;
}
VRect GetCenteredRectInArea(const VRect& area, int32 width, int32 height) {
  VRect rect;
  rect.left = (area.left + area.right) / 2 - width / 2;
  rect.right = rect.left + width;
  rect.top = (area.top + area.bottom) / 2 - height / 2;
  rect.bottom = rect.top + height;
  return rect;
}

VRect GetCropAreaRectInWindow(const VRect& proxy_area_rect_in_window) {
  VRect crop_rect = proxy_area_rect_in_window;
  crop_rect.left = crop_rect.right - (crop_rect.bottom - crop_rect.top);
  crop_rect.left += 2;  // Leaving room for the selection borders.
  crop_rect.right -= 2;
  crop_rect.top += 2;
  crop_rect.bottom -= 2;
  return crop_rect;
}
VRect GetScaleAreaRectInWindow(const VRect& proxy_area_rect_in_window) {
  VRect scale_rect = proxy_area_rect_in_window;
  scale_rect.right =
      GetCropAreaRectInWindow(proxy_area_rect_in_window).left - 5;
  scale_rect.left += 2;  // Leaving room for the selection borders.
  scale_rect.right -= 2;
  scale_rect.top += 2;
  scale_rect.bottom -= 2;
  return scale_rect;
}

//------------------------------------------------------------------------------

std::string DataSizeToString(size_t data_size) {
  if (data_size < 1024) {
    return std::to_string(data_size) + " B";
  }
  if (data_size < 1024 * 1024) {
    return std::to_string(data_size / 1024) + "." +
           std::to_string((data_size % 1024) * 1000 / 1024 / 100) + " KB";
  }
  return std::to_string(data_size / (1024 * 1024)) + "." +
         std::to_string((data_size % (1024 * 1024)) * 1000 / (1024 * 1024) /
                        100) +
         " MB";
}

void SetErrorString(FormatRecordPtr format_record, const std::string& str) {
  if (format_record->errorString != nullptr && str.length() < 255) {
    Str255& errorString = *format_record->errorString;
    errorString[0] = static_cast<unsigned char>(str.length());
    std::copy(str.c_str(), str.c_str() + str.length() + 1,  // Include '\0'.
              reinterpret_cast<char*>(errorString + 1));
    LOG("errorString: " << str);
  } else {
    LOG("errorString (not set): " << str);
  }
}

//------------------------------------------------------------------------------
