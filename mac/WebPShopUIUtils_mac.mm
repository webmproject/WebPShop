// Copyright 2019 Google LLC
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

#include "WebPShopUIUtils_mac.h"

#include <Cocoa/Cocoa.h>
#include <algorithm>

//------------------------------------------------------------------------------

static void DrawTile(CGContextRef cg_context, const NSRect& cg_rect,
                     CGFloat x, CGFloat y, const NSSize& tile_size) {
  const CGFloat right = std::min(x + tile_size.width, cg_rect.size.width);
  const CGFloat bottom = std::min(y + tile_size.height, cg_rect.size.height);
  NSRect tile_rect = NSMakeRect(x, y, right - x, bottom - y);
  tile_rect.origin.y =
      cg_rect.size.height - tile_rect.origin.y - tile_rect.size.height;
  tile_rect.origin.x += cg_rect.origin.x;
  tile_rect.origin.y += cg_rect.origin.y;
  CGContextFillRect(cg_context, tile_rect);
}

void DrawCheckerboard(CGContextRef cg_context, const NSRect& cg_rect,
                      const NSSize& tile_size) {
  CGColorRef white = CGColorCreateGenericRGB(1.0, 1.0, 1.0, 1.0);
  CGContextSetFillColorWithColor(cg_context, white);
  // Draw all tiles of even rows, even columns in white.
  for (CGFloat x = 0; x < cg_rect.size.width; x += 2 * tile_size.width) {
    for (CGFloat y = 0; y < cg_rect.size.height; y += 2 * tile_size.height) {
      DrawTile(cg_context, cg_rect, x, y, tile_size);
    }
  }
  // Draw all tiles of uneven rows, uneven columns in white.
  for (CGFloat x = tile_size.width; x < cg_rect.size.width;
       x += 2 * tile_size.width) {
    for (CGFloat y = tile_size.height; y < cg_rect.size.height;
         y += 2 * tile_size.height) {
      DrawTile(cg_context, cg_rect, x, y, tile_size);
    }
  }
  CGColorRelease(white);

  CGColorRef grey = CGColorCreateGenericRGB(0.8, 0.8, 0.8, 1.0);
  CGContextSetFillColorWithColor(cg_context, grey);
  // Draw all tiles of even rows, uneven columns in grey.
  for (CGFloat x = 0; x < cg_rect.size.width; x += 2 * tile_size.width) {
    for (CGFloat y = tile_size.height; y < cg_rect.size.height;
         y += 2 * tile_size.height) {
      DrawTile(cg_context, cg_rect, x, y, tile_size);
    }
  }
  // Draw all tiles of uneven rows, even columns in grey.
  for (CGFloat x = tile_size.width; x < cg_rect.size.width;
       x += 2 * tile_size.width) {
    for (CGFloat y = 0; y < cg_rect.size.height; y += 2 * tile_size.height) {
      DrawTile(cg_context, cg_rect, x, y, tile_size);
    }
  }
  CGColorRelease(grey);
}

//------------------------------------------------------------------------------
