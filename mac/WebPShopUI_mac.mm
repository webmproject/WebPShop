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

#include <Cocoa/Cocoa.h>
#include <algorithm>
#include <cmath>
#include <string>
#include "AppKit/NSAlert.h"
#include "AppKit/NSWindow.h"
#include "PIUI.h"
#include "WebPShop.h"
#include "WebPShopUI.h"
#include "WebPShopUIDialog_mac.h"
#include "WebPShopUIUtils_mac.h"

//------------------------------------------------------------------------------
// Adobe Photoshop SDK defines but does not implement these for Mac.

int PIDialog::Modal(SPPluginRef plugin, const char*, int ID) { return kDNone; }

bool PICheckBox::GetChecked() {
  return (item != nullptr && [(NSButton*)item state] == NSControlStateValueOn);
}
void PICheckBox::SetChecked(bool state) {
  if (item != nullptr) {
    [(NSButton*)item
        setState:(state ? NSControlStateValueOn : NSControlStateValueOff)];
  }
}
void PICheckBox::SetText(const std::string& in) {
  if (item != nullptr) {
    NSString* nsvalue =
        [NSString stringWithCString:in.c_str()
                           encoding:[NSString defaultCStringEncoding]];
    [(NSButton*)item setTitle:nsvalue];
  }
}

void PIRadioGroup::SetGroupRange(int32 first, int32 last) {
  firstItem = first;
  lastItem = last;
}
void PIRadioGroup::SetSelected(int32 s) {
  for (int32 item_id = firstItem; item_id <= lastItem; ++item_id) {
    NSButton* radio = (NSButton*)((Dialog*)dialog)->GetItemPtr((short)item_id);
    if (radio != nullptr) {
      [radio setState:((item_id == s) ? NSControlStateValueOn
                                      : NSControlStateValueOff)];
    }
  }
}

void PIText::SetText(const std::string& in) {
  if (GetItem() != nullptr) {
    NSString* nsvalue =
        [NSString stringWithCString:in.c_str()
                           encoding:[NSString defaultCStringEncoding]];
    [(NSTextField*)GetItem() setStringValue:nsvalue];
  }
}

//------------------------------------------------------------------------------

void PISlider::SetItem(PIDialogPtr dialog, int16 item_id, int min, int max) {
  item_ = ((Dialog*)dialog)->GetItemPtr(item_id);
  NSSlider* slider = (NSSlider*)item_;
  if (slider != nullptr) {
    [slider setMinValue:min];
    [slider setMaxValue:max];
  }
}
void PISlider::SetValue(int value) {
  NSSlider* slider = (NSSlider*)item_;
  if (slider != nullptr) [slider setIntValue:value];
}
int PISlider::GetValue(void) {
  NSSlider* slider = (NSSlider*)item_;
  if (slider != nullptr) return [slider intValue];
  return -1;
}

//------------------------------------------------------------------------------

void PIIntegerField::SetItem(PIDialogPtr dialog, int16 item_id, int min,
                             int max) {
  dialog_ = dialog;
  item_id_ = item_id;
  min_ = std::min(min, max);
  max_ = std::max(min, max);
}
void PIIntegerField::SetValue(int value) {
  NSTextField* field = (NSTextField*)((Dialog*)dialog_)->GetItemPtr(item_id_);
  if (field != nullptr) [field setIntValue:value];
}
int PIIntegerField::GetValue(void) {
  NSTextField* field = (NSTextField*)((Dialog*)dialog_)->GetItemPtr(item_id_);
  if (field != nullptr) return [field intValue];
  return -1;
}

//------------------------------------------------------------------------------

PIItem WebPShopDialog::GetItem(short item) {
  Dialog* dialog = (Dialog*)GetDialog();
  if (dialog != nullptr) return dialog->GetItemPtr(item);
  return nullptr;
}
void WebPShopDialog::ShowItem(short item) {
  NSView* view = (NSView*)GetItem(item);
  if (view != nullptr) [view setHidden:NO];
}
void WebPShopDialog::HideItem(short item) {
  NSView* view = (NSView*)GetItem(item);
  if (view != nullptr) [view setHidden:YES];
}
void WebPShopDialog::EnableItem(short item) {
  NSButton* view = (NSButton*)GetItem(item);
  if (view != nullptr) [view setEnabled:YES];
}
void WebPShopDialog::DisableItem(short item) {
  NSButton* view = (NSButton*)GetItem(item);
  if (view != nullptr) [view setEnabled:NO];
}

//------------------------------------------------------------------------------

VRect WebPShopDialog::GetProxyAreaRectInWindow(void) {
  WebPShopProxyView* proxy_view = ((Dialog*)GetDialog())->proxy_view;
  if (proxy_view != nullptr) {
    // Return true number of pixels as displayed on screen (Retina).
    NSRect proxy_rect = [proxy_view convertRectToBacking:[proxy_view frame]];
    proxy_rect.size.width -= 2;   // Fix to avoid a small clipping on the
    proxy_rect.origin.x += 1;     // selection's rectangle borders.
    proxy_rect.size.height -= 2;  // The margin in GetScaleAreaRectInWindow()
    proxy_rect.origin.y += 1;     // is apparently not enough on Mac.
    return {(int32)std::lround(proxy_rect.origin.y),
            (int32)std::lround(proxy_rect.origin.x),
            (int32)std::lround(proxy_rect.origin.y + proxy_rect.size.height),
            (int32)std::lround(proxy_rect.origin.x + proxy_rect.size.width)};
  }
  return NullRect();
}

void WebPShopDialog::BeginPainting(PaintingContext* const painting_context) {
  CGContextRef cg_context = [[NSGraphicsContext currentContext] CGContext];
  painting_context->cg_context = (void*)cg_context;
  painting_context->proxy_view = (void*)((Dialog*)GetDialog())->proxy_view;
  CGContextSaveGState(cg_context);
}

void WebPShopDialog::EndPainting(PaintingContext* const painting_context) {
  CGContextRef cg_context = (CGContext*)painting_context->cg_context;
  CGContextRestoreGState(cg_context);
  painting_context->cg_context = nullptr;
  painting_context->proxy_view = nullptr;
}

void WebPShopDialog::ClearRect(const VRect& rect,
                               PaintingContext* const painting_context) {
  // No need to clear anything with Cocoa.
  (void)rect;
  (void)painting_context;
}

void WebPShopDialog::DrawRectBorder(uint8_t r, uint8_t g, uint8_t b,
                                    int left, int top, int right, int bottom,
                                    PaintingContext* const painting_context) {
  if (painting_context->proxy_view == nullptr) return;
  WebPShopProxyView* proxy_view =
      (WebPShopProxyView*)painting_context->proxy_view;

  NSRect border_rect = NSMakeRect(left, top, right - left, bottom - top);
  border_rect = [proxy_view convertTopLeftRectToCGContext:border_rect];

  CGContextRef cg_context = (CGContext*)painting_context->cg_context;
  CGColorRef color =
      CGColorCreateGenericRGB(r / 255.0, g / 255.0, b / 255.0, 1.0);
  CGContextSetStrokeColorWithColor(cg_context, color);
  CGContextStrokeRectWithWidth(cg_context, border_rect, 1.0);
  CGColorRelease(color);
}

//------------------------------------------------------------------------------

bool WebPShopDialog::DisplayImage(const ImageMemoryDesc& image,
                                  const VRect& rect,
                                  DisplayPixelsProc display_pixels_proc,
                                  PaintingContext* const painting_context) {
  // The DisplayPixelsProc function (from Photoshop SDK) sometimes crashes or
  // displays a corrupted image. Core Graphics functions are used instead.
  (void)display_pixels_proc;

  if (image.mode != plugInModeRGBColor) {
    LOG("/!\\ Unhandled image mode (" << image.mode << ").");
    return false;
  } else if (image.num_channels != 3 && image.num_channels != 4) {
    LOG("/!\\ Unhandled channel count (" << image.num_channels << ").");
    return false;
  } else if (painting_context->proxy_view == nullptr) {
    LOG("/!\\ Missing proxy view.");
    return false;
  }
  CGContextRef cg_context = (CGContext*)painting_context->cg_context;
  WebPShopProxyView* proxy_view =
      (WebPShopProxyView*)painting_context->proxy_view;

  CGColorSpaceRef color_space = CGColorSpaceCreateDeviceRGB();
  CGDataProviderRef data_provider = CGDataProviderCreateWithData(
      /*data_to_release=*/nullptr, image.pixels.data,
      (image.pixels.rowBits / 8) * image.height,
      /*release_function=*/nullptr);

  const CGImageAlphaInfo alpha_info =
      (image.num_channels == 4) ? kCGImageAlphaLast : kCGImageAlphaNone;
  CGImageRef cg_image =
      CGImageCreate(image.width, image.height, image.pixels.depth,
                    image.pixels.colBits, image.pixels.rowBits / 8, color_space,
                    kCGBitmapByteOrderDefault | alpha_info, data_provider,
                    /*color_mapping_function=*/nullptr,
                    /*should_interpolate=*/false, kCGRenderingIntentDefault);

  NSRect cg_image_rect = NSMakeRect(rect.left, rect.top, rect.right - rect.left,
                                    rect.bottom - rect.top);
  cg_image_rect = [proxy_view convertTopLeftRectToCGContext:cg_image_rect];

  // Display a checkerboard if the image is transparent.
  if (image.num_channels == 4) {
    const NSSize tile_size =
        [proxy_view convertSizeFromBacking:NSMakeSize(8, 8)];
    DrawCheckerboard(cg_context, cg_image_rect, tile_size);

    // Transparency layer to render the image on top of the checkerboard.
    // CGContextFlush(cg_context) also works but it's probably slower.
    CGContextBeginTransparencyLayerWithRect(cg_context, cg_image_rect, NULL);
  }

  CGContextDrawImage(cg_context, cg_image_rect, cg_image);

  if (image.num_channels == 4) CGContextEndTransparencyLayer(cg_context);
  CGImageRelease(cg_image);
  CGDataProviderRelease(data_provider);
  CGColorSpaceRelease(color_space);
  return true;
}

//------------------------------------------------------------------------------

void WebPShopDialog::TriggerRepaint() {
  WebPShopProxyView* proxy_view = ((Dialog*)GetDialog())->proxy_view;
  if (proxy_view != nullptr) [proxy_view display];
}

//------------------------------------------------------------------------------
// Encoding settings UI entry point

int WebPShopDialog::Modal(SPPluginRef plugin, const char*, int ID) {
  SetID(ID);
  SetPluginRef(plugin);

  Dialog dialog;
  SetDialog((PIDialogPtr)&dialog);
  WebPShopDelegate* const delegate =
      [[[WebPShopDelegate alloc] init] autorelease];
  [delegate setDialog:this];

  NSWindow* const window = dialog.CreateWindow(delegate);
  [delegate setWindow:window];
  [dialog.proxy_view setDialog:this];
  Init();

  // This will return only once the window is closed.
  [[NSApplication sharedApplication] runModalForWindow:window];

  // 'delegate' is autoreleased and 'window' is releasedWhenClosed.
  SetDialog(nullptr);
  return dialog.clicked_button;
}

//------------------------------------------------------------------------------
// About box

void DoAboutBox(SPPluginRef plugin_ref) {
  NSAlert* const alert = [[NSAlert alloc] init];

  [alert addButtonWithTitle:@"OK"];
  [alert addButtonWithTitle:@"developers.google.com/speed/webp"];

  [alert setMessageText:@"About WebPShop"];
  [alert setInformativeText:
             @"WebPShop 0.3.3\nWebP 1.2.0\nA Photoshop plug-in for reading "
             @"and writing WebP files.\nCopyright 2019-2021 Google LLC."];
  [alert setAlertStyle:NSAlertStyleInformational];

  const NSModalResponse buttonPressed = [alert runModal];
  if (buttonPressed == NSAlertSecondButtonReturn) {
    sPSFileList->BrowseUrl("https://developers.google.com/speed/webp");
  }

  [alert release];
}
