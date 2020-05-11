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

#include "WebPShopUIDialog_mac.h"

#include <Cocoa/Cocoa.h>
#include <cmath>
#include <utility>
#include "AppKit/NSWindow.h"

//------------------------------------------------------------------------------

@implementation WebPShopProxyView
- (id)initWithFrame:(NSRect)frame {
  self = [super initWithFrame:frame];
  return self;
}
- (void)drawRect:(NSRect)rect {
  if (self.dialog != nullptr) self.dialog->PaintProxy();
  [super drawRect:rect];
}
- (void)mouseDragged:(NSEvent *)event {
  NSPoint cursor_position = [event locationInWindow];
  // Cocoa uses lower-left-based coordinates.
  cursor_position = [self convertPoint:cursor_position fromView:nil];
  cursor_position.y = [self frame].size.height - cursor_position.y - 1;
  cursor_position = [self convertPoint:cursor_position toView:nil];
  // Return true number of pixels as displayed on screen (Retina).
  cursor_position = [self convertPointToBacking:cursor_position];
  self.dialog->OnMouseMove((int)std::lround(cursor_position.x),
                           (int)std::lround(cursor_position.y),
                           /*left_button_is_held_down=*/true);
}
- (void)mouseDown:(NSEvent *)event {
  [self mouseDragged:event];
}
- (NSRect)convertTopLeftRectToCGContext:(NSRect)rect {
  const NSRect proxy_rect = [self convertRectToBacking:[self frame]];
  // Core Graphics functions draw in the interior coordinate system of the
  // current view, not in the whole window coordinate system.
  rect.origin.x -= proxy_rect.origin.x;
  rect.origin.y -= proxy_rect.origin.y;
  // Cocoa is bottom-left based.
  rect.origin.y = proxy_rect.size.height - rect.origin.y - rect.size.height;
  // Core Graphics functions take local space coordinates, not the true number
  // of pixels displayed on screen.
  return [self convertRectFromBacking:rect];
}
@end

//------------------------------------------------------------------------------

@implementation WebPShopDelegate
- (id)init {
  self = [super init];
  return self;
}
- (void)clickOk:(id)sender {
  ((Dialog*)self.dialog->GetDialog())->clicked_button = kDOK;
  [self.window performClose:sender];
}
- (void)clickCancel:(id)sender {
  ((Dialog*)self.dialog->GetDialog())->clicked_button = kDCancel;
  [self.window performClose:sender];
}
- (void)controlTextDidChange:(NSNotification *)obj {
  [self notified:[obj object]];
}
- (void)notified:(NSObject *)sender {
  self.dialog->Notify(
      ((Dialog*)self.dialog->GetDialog())->GetItemId((PIItem)sender));
}
- (void)windowWillClose:(NSNotification *)notification {
  [NSApp stopModal];
}
@end

//------------------------------------------------------------------------------

void Dialog::Set(short item_id, void* const item_ptr) {
  if (item_id == kDNone) return;
  item_id_to_ptr.insert(std::make_pair(item_id, (PIItem)item_ptr));
}
PIItem Dialog::GetItemPtr(short item_id) {
  const auto item = item_id_to_ptr.find(item_id);
  if (item != item_id_to_ptr.end()) return item->second;
  return nullptr;
}
short Dialog::GetItemId(PIItem item_ptr) {
  for (const auto& item : item_id_to_ptr) {
    if (item.second == item_ptr) return item.first;
  }
  return kDNone;
}

//------------------------------------------------------------------------------

NSWindow* Dialog::CreateWindow(WebPShopDelegate* const delegate) {
  LOG("UI dialog window creation");
  NSWindow* window = [[NSWindow alloc]
      initWithContentRect:NSMakeRect(0, 0, 369, 338)
                styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
                  backing:NSBackingStoreBuffered
                    defer:NO];
  [window setTitle:@"WebPShop"];
  [window setReleasedWhenClosed:YES];
  [window setDelegate:delegate];

  LOG("  General elements");
  Set(kDWebPText,
      settings_text = [NSTextField labelWithString:@"WebP settings"]);
  [[window contentView] addSubview:settings_text];
  [settings_text setFrame:NSMakeRect(6, 317, 200, 16)];
  [settings_text setFont:[NSFont systemFontOfSize:11]];

  Set(kDOK, ok_button = [NSButton buttonWithTitle:@"OK"
                                           target:delegate
                                           action:@selector(clickOk:)]);
  [[window contentView] addSubview:ok_button];
  [ok_button setFrame:NSMakeRect(300, 308, 70, 25)];

  Set(kDCancel,
      cancel_button = [NSButton buttonWithTitle:@"Cancel"
                                         target:delegate
                                         action:@selector(clickCancel:)]);
  [[window contentView] addSubview:cancel_button];
  [cancel_button setFrame:NSMakeRect(300, 278, 70, 25)];

  LOG("  Quality elements");
  Set(kDNone,
      quality_box = [[NSBox alloc] initWithFrame:NSMakeRect(5, 248, 204, 70)]);
  [[window contentView] addSubview:quality_box];
  [quality_box setTitle:@"Quality"];

  Set(kDQualitySlider,
      quality_slider =
          [NSSlider sliderWithTarget:delegate action:@selector(notified:)]);
  [[window contentView] addSubview:quality_slider];
  [quality_slider setFrame:NSMakeRect(40, 278, 100, 20)];

  Set(kDQualityField, quality_field = [NSTextField textFieldWithString:@"75"]);
  [[window contentView] addSubview:quality_field];
  [quality_field setFrame:NSMakeRect(170, 278, 30, 22)];
  [quality_field setDelegate:delegate];

  Set(kDNone, quality_text_smallest =
                  [NSTextField labelWithString:@"Lossy"]);
  [[window contentView] addSubview:quality_text_smallest];
  [quality_text_smallest setFrame:NSMakeRect(20, 258, 80, 16)];
  [quality_text_smallest setFont:[NSFont systemFontOfSize:11]];

  Set(kDNone,
      quality_text_lossless = [NSTextField labelWithString:@"Lossless"]);
  [[window contentView] addSubview:quality_text_lossless];
  [quality_text_lossless setFrame:NSMakeRect(120, 258, 80, 16)];
  [quality_text_lossless setFont:[NSFont systemFontOfSize:11]];

  LOG("  Compression elements");
  Set(kDNone, compression_box =
                  [[NSBox alloc] initWithFrame:NSMakeRect(210, 248, 95, 80)]);
  [[window contentView] addSubview:compression_box];
  [compression_box setTitle:@"Compression"];

  Set(kDCompressionFastest, compression_radio_button_fastest = [NSButton
                                radioButtonWithTitle:@"Fastest"
                                              target:delegate
                                              action:@selector(notified:)]);
  [[window contentView] addSubview:compression_radio_button_fastest];
  [compression_radio_button_fastest setFrame:NSMakeRect(220, 293, 66, 20)];
  [compression_radio_button_fastest setFont:[NSFont systemFontOfSize:11]];

  Set(kDCompressionDefault, compression_radio_button_default = [NSButton
                                radioButtonWithTitle:@"Default"
                                              target:delegate
                                              action:@selector(notified:)]);
  [[window contentView] addSubview:compression_radio_button_default];
  [compression_radio_button_default setFrame:NSMakeRect(220, 273, 66, 20)];
  [compression_radio_button_default setFont:[NSFont systemFontOfSize:11]];

  Set(kDCompressionSlowest, compression_radio_button_smallest = [NSButton
                                radioButtonWithTitle:@"Slowest"
                                              target:delegate
                                              action:@selector(notified:)]);
  [[window contentView] addSubview:compression_radio_button_smallest];
  [compression_radio_button_smallest setFrame:NSMakeRect(220, 253, 66, 20)];
  [compression_radio_button_smallest setFont:[NSFont systemFontOfSize:11]];
  
  LOG("  Metadata elements");
  Set(kDNone, metadata_box =
                  [[NSBox alloc] initWithFrame:NSMakeRect(5, 195, 359, 50)]);
  [[window contentView] addSubview:metadata_box];
  [metadata_box setTitle:@"Metadata"];

  Set(kDKeepExif, metadata_exif_checkbox =
                      [NSButton checkboxWithTitle:@"Keep EXIF"
                                           target:delegate
                                           action:@selector(notified:)]);
  [[window contentView] addSubview:metadata_exif_checkbox];
  [metadata_exif_checkbox setFrame:NSMakeRect(15, 205, 80, 22)];
  [metadata_exif_checkbox setFont:[NSFont systemFontOfSize:11]];

  Set(kDKeepXmp, metadata_xmp_checkbox =
                      [NSButton checkboxWithTitle:@"Keep XMP"
                                           target:delegate
                                           action:@selector(notified:)]);
  [[window contentView] addSubview:metadata_xmp_checkbox];
  [metadata_xmp_checkbox setFrame:NSMakeRect(100, 205, 80, 22)];
  [metadata_xmp_checkbox setFont:[NSFont systemFontOfSize:11]];

  Set(kDKeepColorProfile, metadata_iccp_checkbox =
                      [NSButton checkboxWithTitle:@"Keep ICC"
                                           target:delegate
                                           action:@selector(notified:)]);
  [[window contentView] addSubview:metadata_iccp_checkbox];
  [metadata_iccp_checkbox setFrame:NSMakeRect(185, 205, 80, 22)];
  [metadata_iccp_checkbox setFont:[NSFont systemFontOfSize:11]];

  Set(kDLoopForever, metadata_loop_checkbox =
                      [NSButton checkboxWithTitle:@"Loop"
                                           target:delegate
                                           action:@selector(notified:)]);
  [[window contentView] addSubview:metadata_loop_checkbox];
  [metadata_loop_checkbox setFrame:NSMakeRect(270, 205, 80, 22)];
  [metadata_loop_checkbox setFont:[NSFont systemFontOfSize:11]];

  LOG("  Proxy");
  Set(kDNone,
      proxy_box = [[NSBox alloc] initWithFrame:NSMakeRect(5, 5, 359, 180)]);
  [[window contentView] addSubview:proxy_box];
  [proxy_box setTitle:@""];  // The checkbox replaces the title.

  Set(kDProxyCheckbox,
      proxy_checkbox = [NSButton checkboxWithTitle:@"Preview"
                                            target:delegate
                                            action:@selector(notified:)]);
  [[window contentView] addSubview:proxy_checkbox];
  [proxy_checkbox setFrame:NSMakeRect(20, 170, 160, 22)];
  [proxy_checkbox setFont:[NSFont systemFontOfSize:11]];

  const CGFloat proxy_padding = 10;
  NSRect proxy_view_rect =
      NSMakeRect([proxy_box frame].origin.x + proxy_padding,
                 [proxy_box frame].origin.y + proxy_padding,
                 [proxy_box frame].size.width - 2 * proxy_padding,
                 [proxy_box frame].size.height - 2 * proxy_padding - 10);
  Set(kDNone,
      proxy_view = [[WebPShopProxyView alloc] initWithFrame:proxy_view_rect]);
  [[window contentView] addSubview:proxy_view];

  LOG("  Animation");
  Set(kDFrameText, frame_text = [NSTextField labelWithString:@"Frame:"]);
  [[window contentView] addSubview:frame_text];
  [frame_text setFrame:NSMakeRect(185, 169, 40, 22)];
  [frame_text setFont:[NSFont systemFontOfSize:11]];

  Set(kDFrameSlider,
      frame_index_slider =
          [NSSlider sliderWithTarget:delegate action:@selector(notified:)]);
  [[window contentView] addSubview:frame_index_slider];
  [frame_index_slider setFrame:NSMakeRect(225, 173, 50, 22)];

  Set(kDFrameField, frame_index_field = [NSTextField textFieldWithString:@"1"]);
  [[window contentView] addSubview:frame_index_field];
  [frame_index_field setFrame:NSMakeRect(280, 173, 30, 22)];
  [frame_index_field setDelegate:delegate];

  Set(kDFrameDurationText,
      frame_duration_text = [NSTextField labelWithString:@"100 ms"]);
  [[window contentView] addSubview:frame_duration_text];
  [frame_duration_text setFrame:NSMakeRect(315, 169, 40, 22)];
  [frame_duration_text setFont:[NSFont systemFontOfSize:11]];
  [frame_duration_text setAlignment:NSTextAlignmentCenter];

  return window;
}

//------------------------------------------------------------------------------
