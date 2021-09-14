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

#ifndef __WebPShopUI_H__
#define __WebPShopUI_H__

#include "PIUI.h"
#include "WebPShop.h"

//------------------------------------------------------------------------------
// UI element id (also defined in win/WebPShop.rc)

const int16 kDNone = -1;

const int16 kDOK = 1;
const int16 kDCancel = 2;
const int16 kDWebPText = 5;
const int16 kDQualitySlider = 11;
const int16 kDQualityField = 12;
const int16 kDCompressionFastest = 21;
const int16 kDCompressionDefault = 22;
const int16 kDCompressionSlowest = 23;
const int16 kDKeepExif = 33;
const int16 kDKeepXmp = 34;
const int16 kDKeepColorProfile = 35;
const int16 kDLoopForever = 38;
const int16 kDProxy = 41;
const int16 kDProxyCheckbox = 42;
const int16 kDFrameText = 45;
const int16 kDFrameSlider = 46;
const int16 kDFrameField = 47;
const int16 kDFrameDurationText = 48;

const int16 kDAboutWebPLink = 10;

//------------------------------------------------------------------------------
// Platform-dependent painting context

struct PaintingContext {
#ifdef __PIMac__
  void* cg_context;  // CGContextRef instance
  void* proxy_view;  // WebPShopProxyView instance
#else
  PAINTSTRUCT ps;
  HDC hDC;
#endif
};

//------------------------------------------------------------------------------
// Utils

VRect NullRect();
VRect GetCenteredRectInArea(const VRect& area, int32 width, int32 height);
VRect GetCropAreaRectInWindow(const VRect& proxy_area_rect_in_window);
VRect GetScaleAreaRectInWindow(const VRect& proxy_area_rect_in_window);
std::string DataSizeToString(size_t data_size);
void SetErrorString(FormatRecordPtr format_record, const std::string& str);

//------------------------------------------------------------------------------
// UI elements (inspired from Adobe SDK)

class PISlider {
  PIItem item_;

 public:
  PISlider() : item_(NULL) {}
  ~PISlider() {}

  void SetItem(PIDialogPtr dialog, int16 item_id, int min, int max);
  void SetValue(int value);
  int GetValue(void);
  void SetValueIfDifferent(int value) {
    if (GetValue() != value) SetValue(value);
  }
};

class PIIntegerField {
  PIDialogPtr dialog_;
  int16 item_id_;
  int min_, max_;

 public:
  PIIntegerField() : dialog_(nullptr), item_id_(-1), min_(0), max_(0) {}
  ~PIIntegerField() {}

  void SetItem(PIDialogPtr dialog, int16 item_id, int min, int max);
  void SetValue(int value);
  int GetValue(void);
  void SetValueIfDifferent(int value) {
    if (GetValue() != value) SetValue(value);
  }
};

//------------------------------------------------------------------------------
// UI window and element instances

class WebPShopDialog : public PIDialog {
 private:
  // UI elements
  PIText webp_text_;
  PISlider quality_slider_;
  PIIntegerField quality_field_;
  PIRadioGroup compression_radio_group_;
  PICheckBox keep_exif_checkbox_;
  PICheckBox keep_xmp_checkbox_;
  PICheckBox keep_color_profile_checkbox_;
  PICheckBox loop_forever_checkbox_;
  PICheckBox proxy_checkbox_;
  PISlider frame_slider_;
  PIIntegerField frame_field_;
  PIText frame_duration_text_;

  // Settings
  WriteConfig write_config_;
  const Metadata* metadata_;

  // Currently displayed
  size_t frame_index_;
  VRect selection_in_compressed_frame_;

  // Before encoding
  const std::vector<FrameMemoryDesc>& original_frames_;
  const bool original_frames_were_converted_to_8b_;
  // After encoding
  WebPData* const encoded_data_;
  // After decoding (for proxy)
  std::vector<FrameMemoryDesc> compressed_frames_;
  std::vector<FrameMemoryDesc> scaled_compressed_frames_;
  ImageMemoryDesc cropped_compressed_frame_;
  bool update_cropped_compressed_frame_;  // If frame or selection changed.

  // Adobe SDK portable display function
  DisplayPixelsProc display_pixels_proc_;

  // Clear
  void DiscardEncodedData(void);
  void OnError(void);

  // Platform-dependent
  PIItem GetItem(short item);
  void ShowItem(short item);
  void HideItem(short item);
  void EnableItem(short item);
  void DisableItem(short item);
  VRect GetProxyAreaRectInWindow(void);
  void BeginPainting(PaintingContext* const painting_context);
  void EndPainting(PaintingContext* const painting_context);
  void ClearRect(const VRect& rect, PaintingContext* const painting_context);
  void DrawRectBorder(uint8_t r, uint8_t g, uint8_t b,
                      int left, int top, int right, int bottom,
                      PaintingContext* const painting_context);
  bool DisplayImage(const ImageMemoryDesc& image, const VRect& rect,
                    DisplayPixelsProc display_pixels_proc,
                    PaintingContext* const painting_context);
  void TriggerRepaint();

 public:
  WebPShopDialog(const WriteConfig& write_config,
                 const Metadata metadata[Metadata::kNum],
                 const std::vector<FrameMemoryDesc>& original_frames,
                 bool original_frames_were_converted_to_8b,
                 WebPData* const encoded_data,
                 DisplayPixelsProc display_pixels_proc)
      : PIDialog(),
        webp_text_(),
        quality_slider_(),
        quality_field_(),
        compression_radio_group_(),
        keep_exif_checkbox_(),
        keep_xmp_checkbox_(),
        keep_color_profile_checkbox_(),
        loop_forever_checkbox_(),
        proxy_checkbox_(),
        frame_slider_(),
        frame_field_(),
        frame_duration_text_(),
        write_config_(write_config),
        metadata_(metadata),
        frame_index_(0),
        selection_in_compressed_frame_(),
        original_frames_(original_frames),
        original_frames_were_converted_to_8b_(
            original_frames_were_converted_to_8b),
        encoded_data_(encoded_data),
        compressed_frames_(),
        scaled_compressed_frames_(),
        cropped_compressed_frame_(),
        update_cropped_compressed_frame_(true),
        display_pixels_proc_(display_pixels_proc) {}
  ~WebPShopDialog() { DeallocateCompressedFrames(); }

  void DeallocateCompressedFrames(void);
  const WriteConfig& GetWriteConfig(void) const { return write_config_; }

  void Init(void) override;

  void ForceRepaint(void);
  void ClearProxyArea(void);
  void PaintProxy(void);

  void Notify(int32 item) override;
  void OnMouseMove(int x, int y, bool left_button_is_held_down);

  // Platform-dependent callback registration.
  int Modal(SPPluginRef plugin, const char* name, int ID) override;
};

#endif  // __WebPShopUI_H__
