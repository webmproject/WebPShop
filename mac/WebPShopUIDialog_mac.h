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

#ifndef __WebPShopUIDialog_mac_H__
#define __WebPShopUIDialog_mac_H__

#include <Cocoa/Cocoa.h>
#include <map>
#include "AppKit/NSWindow.h"
#include "PIUI.h"

#include "WebPShop.h"
#include "WebPShopUI.h"

//------------------------------------------------------------------------------
// WebPShopProxyView is the UI element containing the preview area in the
// encoding settings window. It inherits NSView for preview image drawing and
// events.

@interface WebPShopProxyView : NSView
@property(assign) WebPShopDialog *dialog;
- (NSRect)convertTopLeftRectToCGContext:(NSRect)rect;
@end

//------------------------------------------------------------------------------
// WebPShopDelegate handles callbacks sent by UI elements.

@interface WebPShopDelegate : NSObject <NSWindowDelegate, NSTextFieldDelegate>
@property(assign) NSWindow *window;
@property(assign) WebPShopDialog *dialog;
- (void)clickOk:(id)sender;
- (void)clickCancel:(id)sender;
- (void)notified:(NSObject *)sender;
@end

//------------------------------------------------------------------------------
// Contains each UI element with its matching id.

struct Dialog {
 public:
  void Set(short item_id, void* const item_ptr);
  PIItem GetItemPtr(short item_id);
  short GetItemId(PIItem item_ptr);
  NSWindow* CreateWindow(WebPShopDelegate* const delegate);

  NSTextField* settings_text = nullptr;
  NSButton* ok_button = nullptr;
  NSButton* cancel_button = nullptr;
  int clicked_button = kDCancel;

  NSBox* quality_box = nullptr;
  NSSlider* quality_slider = nullptr;
  NSTextField* quality_field = nullptr;
  NSTextField* quality_text_smallest = nullptr;
  NSTextField* quality_text_lossless = nullptr;

  NSBox* compression_box = nullptr;
  NSButton* compression_radio_button_fastest = nullptr;
  NSButton* compression_radio_button_default = nullptr;
  NSButton* compression_radio_button_smallest = nullptr;

  NSBox* metadata_box = nullptr;
  NSButton* metadata_exif_checkbox = nullptr;
  NSButton* metadata_xmp_checkbox = nullptr;
  NSButton* metadata_iccp_checkbox = nullptr;
  NSButton* metadata_loop_checkbox = nullptr;

  NSBox* proxy_box = nullptr;
  NSButton* proxy_checkbox = nullptr;
  WebPShopProxyView* proxy_view = nullptr;

  NSTextField* frame_text = nullptr;
  NSSlider* frame_index_slider = nullptr;
  NSTextField* frame_index_field = nullptr;
  NSTextField* frame_duration_text = nullptr;

 private:
  std::map<short, PIItem> item_id_to_ptr;
};

//------------------------------------------------------------------------------

#endif  // __WebPShopUIDialog_mac_H__
