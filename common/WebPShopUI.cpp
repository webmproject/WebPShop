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

#include "WebPShopUI.h"

#include <string>

#include "PIUI.h"
#include "WebPShop.h"

//------------------------------------------------------------------------------

bool DoUI(WriteConfig* const write_config,
          const Metadata metadata[Metadata::kNum], SPPluginRef plugin_ref,
          const std::vector<FrameMemoryDesc>& original_frames,
          WebPData* const encoded_data, DisplayPixelsProc display_pixels_proc) {
  WebPShopDialog dialog(*write_config, metadata, original_frames, encoded_data,
                        display_pixels_proc);
  int result = dialog.Modal(plugin_ref, NULL, 16090);
  if (result == kDOK) *write_config = dialog.GetWriteConfig();
  dialog.DeallocateCompressedFrames();
  return result == kDOK;
}

//------------------------------------------------------------------------------

void WebPShopDialog::DeallocateCompressedFrames(void) {
  ClearFrameVector(&compressed_frames_);
  ClearFrameVector(&scaled_compressed_frames_);
  DeallocateImage(&cropped_compressed_frame_);
}

void WebPShopDialog::DiscardEncodedData(void) {
  WebPDataClear(encoded_data_);
  DeallocateCompressedFrames();
  update_cropped_compressed_frame_ = true;
}

void WebPShopDialog::OnError(void) {
  DiscardEncodedData();
  selection_in_compressed_frame_ = NullRect();

  write_config_.display_proxy = false;
  proxy_checkbox_.SetChecked(write_config_.display_proxy);
  proxy_checkbox_.SetText("Preview is unavailable.");
}

//------------------------------------------------------------------------------

void WebPShopDialog::Init(void) {
  PIDialogPtr dialog = GetDialog();

  // UI elements initialization.
  webp_text_.SetItem(GetItem(kDWebPText));

  quality_slider_.SetItem(dialog, kDQualitySlider, 0, 100);
  quality_field_.SetItem(dialog, kDQualityField, 0, 100);

  compression_radio_group_.SetDialog(dialog);
  compression_radio_group_.SetGroupRange(kDCompressionFastest,
                                         kDCompressionSlowest);

  keep_exif_checkbox_.SetItem(GetItem(kDKeepExif));
  keep_xmp_checkbox_.SetItem(GetItem(kDKeepXmp));
  keep_color_profile_checkbox_.SetItem(GetItem(kDKeepColorProfile));
  loop_forever_checkbox_.SetItem(GetItem(kDLoopForever));

  proxy_checkbox_.SetItem(GetItem(kDProxyCheckbox));

  frame_duration_text_.SetItem(GetItem(kDFrameDurationText));

  // The number of compressed frames can not be known yet.
  HideItem(kDLoopForever);
  HideItem(kDFrameText);
  HideItem(kDFrameSlider);
  HideItem(kDFrameField);
  HideItem(kDFrameDurationText);

  // Data initialization.
  frame_index_ = 0;
  selection_in_compressed_frame_ = NullRect();

  ClearFrameVector(&compressed_frames_);
  ClearFrameVector(&scaled_compressed_frames_);
  DeallocateImage(&cropped_compressed_frame_);
  update_cropped_compressed_frame_ = true;

  // UI elements default values.
  if (write_config_.animation) {
    webp_text_.SetText("Animated WebP settings:");
  } else {
    webp_text_.SetText("WebP settings:");
  }

  quality_slider_.SetValue(write_config_.quality);
  quality_field_.SetValue(write_config_.quality);

  if (write_config_.compression == Compression::FASTEST) {
    compression_radio_group_.SetSelected(kDCompressionFastest);
  } else if (write_config_.compression == Compression::DEFAULT) {
    compression_radio_group_.SetSelected(kDCompressionDefault);
  } else {
    compression_radio_group_.SetSelected(kDCompressionSlowest);
  }

  if (metadata_[Metadata::kEXIF].chunk.bytes != nullptr &&
      metadata_[Metadata::kEXIF].chunk.size > 0) {
    keep_exif_checkbox_.SetChecked(write_config_.keep_exif);
    EnableItem(kDKeepExif);
  } else {
    keep_exif_checkbox_.SetChecked(false);
    DisableItem(kDKeepExif);
  }
  if (metadata_[Metadata::kXMP].chunk.bytes != nullptr &&
      metadata_[Metadata::kXMP].chunk.size > 0) {
    keep_xmp_checkbox_.SetChecked(write_config_.keep_xmp);
    EnableItem(kDKeepXmp);
  } else {
    keep_xmp_checkbox_.SetChecked(false);
    DisableItem(kDKeepXmp);
  }
  if (metadata_[Metadata::kICCP].chunk.bytes != nullptr &&
      metadata_[Metadata::kICCP].chunk.size > 0) {
    keep_color_profile_checkbox_.SetChecked(write_config_.keep_color_profile);
    EnableItem(kDKeepColorProfile);
  } else {
    keep_color_profile_checkbox_.SetChecked(false);
    DisableItem(kDKeepColorProfile);
  }

  loop_forever_checkbox_.SetChecked(write_config_.loop_forever);
  if (write_config_.animation) {
    ShowItem(kDLoopForever);
  } else {
    HideItem(kDLoopForever);
  }

  proxy_checkbox_.SetChecked(write_config_.display_proxy);
}

//------------------------------------------------------------------------------

void WebPShopDialog::ForceRepaint() {
  TriggerRepaint();

  if (write_config_.display_proxy) {
    if (encoded_data_->bytes == nullptr) {
      // Show that computation is running.
      proxy_checkbox_.SetText("Preview: ...");
    }
  } else {
    proxy_checkbox_.SetText("Preview");
  }
}

void WebPShopDialog::ClearProxyArea() {
  PaintingContext painting_context;
  BeginPainting(&painting_context);
  ClearRect(GetProxyAreaRectInWindow(), &painting_context);
  EndPainting(&painting_context);

  HideItem(kDFrameText);
  HideItem(kDFrameSlider);
  HideItem(kDFrameField);
  HideItem(kDFrameDurationText);
}

//------------------------------------------------------------------------------

void WebPShopDialog::PaintProxy(void) {
  if (!write_config_.display_proxy) {
    proxy_checkbox_.SetText("Preview");
    ClearProxyArea();
    return;
  }
  if (display_pixels_proc_ == nullptr || encoded_data_ == nullptr) {
    LOG("/!\\ No displayPixels available or no data buffer.");
    OnError();
    ClearProxyArea();
    return;
  }

  PIDialogPtr dialog = GetDialog();
  const VRect proxy_area = GetProxyAreaRectInWindow();
  const VRect scale_area = GetScaleAreaRectInWindow(proxy_area);
  const VRect crop_area = GetCropAreaRectInWindow(proxy_area);

  if (encoded_data_->bytes == nullptr) {
    if (write_config_.animation) {
      if (original_frames_.empty()) {
        LOG("/!\\ No frame to encode.");
        OnError();
        ClearProxyArea();
        return;
      }

      if (!EncodeAllFrames(original_frames_, write_config_, encoded_data_) ||
          encoded_data_->size == 0) {
        LOG("/!\\ Encoding failed.");
        OnError();
        ClearProxyArea();
        return;
      }

      if (!EncodeMetadata(write_config_, metadata_, encoded_data_)) {
        LOG("/!\\ Metadata muxing failed.");
        OnError();
        ClearProxyArea();
        return;
      }

      // The number of original and compressed frames might differ
      // if there are identical ones; don't check equality.
      if (!DecodeAllFrames(*encoded_data_, &compressed_frames_) ||
          compressed_frames_.empty()) {
        LOG("/!\\ Decoding failed.");
        OnError();
        ClearProxyArea();
        return;
      }

      // Number of frames might also change between qualities.
      frame_slider_.SetItem(dialog, kDFrameSlider, 0,
                            (int)compressed_frames_.size() - 1);
      frame_field_.SetItem(dialog, kDFrameField, 1,
                           (int)compressed_frames_.size());

      if (frame_index_ >= compressed_frames_.size()) {
        frame_index_ = compressed_frames_.size() - 1;
      }

      frame_slider_.SetValue((int)frame_index_);
      frame_field_.SetValue((int)frame_index_ + 1);
    } else {  // !write_config_.animation
      if (original_frames_.size() != 1) {
        LOG("/!\\ Need exactly one image to encode.");
        OnError();
        ClearProxyArea();
        return;
      }

      const ImageMemoryDesc& original_image = original_frames_.front().image;
      if (!EncodeOneImage(original_image, write_config_, encoded_data_) ||
          encoded_data_->size == 0) {
        LOG("/!\\ Encoding failed.");
        OnError();
        ClearProxyArea();
        return;
      }

      if (!EncodeMetadata(write_config_, metadata_, encoded_data_)) {
        LOG("/!\\ Metadata muxing failed.");
        OnError();
        ClearProxyArea();
        return;
      }

      ResizeFrameVector(&compressed_frames_, 1);

      ImageMemoryDesc& compressed_frame = compressed_frames_.front().image;
      if (!DecodeOneImage(*encoded_data_, &compressed_frame) ||
          (compressed_frame.width != original_image.width) ||
          (compressed_frame.height != original_image.height)) {
        LOG("/!\\ Decoding failed.");
        OnError();
        ClearProxyArea();
        return;
      }

      frame_index_ = 0;
    }

    const bool compressed_image_fits_proxy_area =
        (compressed_frames_.front().image.width <= GetWidth(proxy_area)) &&
        (compressed_frames_.front().image.height <= GetHeight(proxy_area));

    if (compressed_image_fits_proxy_area) {
      ClearFrameVector(&scaled_compressed_frames_);
      DeallocateImage(&cropped_compressed_frame_);
      update_cropped_compressed_frame_ = false;
    } else {
      int32 scaled_width = compressed_frames_.front().image.width;
      int32 scaled_height = compressed_frames_.front().image.height;
      ScaleToFit(&scaled_width, &scaled_height, GetWidth(scale_area),
                 GetHeight(scale_area));
      ResizeFrameVector(&scaled_compressed_frames_, compressed_frames_.size());
      for (size_t i = 0; i < scaled_compressed_frames_.size(); ++i) {
        Scale(compressed_frames_[i].image, &scaled_compressed_frames_[i].image,
              (size_t)scaled_width, (size_t)scaled_height);
      }
      update_cropped_compressed_frame_ = true;
    }
  }

  proxy_checkbox_.SetText("Preview: " + DataSizeToString(encoded_data_->size));

  if (update_cropped_compressed_frame_) {
    ImageMemoryDesc& compressed_frame = compressed_frames_[frame_index_].image;

    int32 cropped_width = compressed_frame.width;
    int32 cropped_height = compressed_frame.height;
    CropToFit(&cropped_width, &cropped_height, 0, 0, GetWidth(crop_area),
              GetHeight(crop_area));

    if (GetWidth(selection_in_compressed_frame_) != cropped_width ||
        GetHeight(selection_in_compressed_frame_) != cropped_height) {
      selection_in_compressed_frame_.left = 0;
      selection_in_compressed_frame_.right = cropped_width;
      selection_in_compressed_frame_.top = 0;
      selection_in_compressed_frame_.bottom = cropped_height;
    }

    Crop(compressed_frame, &cropped_compressed_frame_,
         (size_t)GetWidth(selection_in_compressed_frame_),
         (size_t)GetHeight(selection_in_compressed_frame_),
         (size_t)selection_in_compressed_frame_.left,
         (size_t)selection_in_compressed_frame_.top);
    update_cropped_compressed_frame_ = false;
  }

  if (write_config_.animation) {
    ShowItem(kDFrameText);
    ShowItem(kDFrameSlider);
    ShowItem(kDFrameField);
    ShowItem(kDFrameDurationText);
    frame_duration_text_.SetText(
        std::to_string(compressed_frames_[frame_index_].duration_ms) + " ms");
  }

  PaintingContext painting_context;
  BeginPainting(&painting_context);
  ClearRect(proxy_area, &painting_context);

  if (scaled_compressed_frames_.empty()) {
    ImageMemoryDesc& compressed_frame = compressed_frames_[frame_index_].image;

    const VRect compressed_frame_rect = GetCenteredRectInArea(
        proxy_area, compressed_frame.width, compressed_frame.height);
    if (!DisplayImage(compressed_frame, compressed_frame_rect,
                      display_pixels_proc_, &painting_context)) {
      OnError();
      ClearProxyArea();
      return;
    }
  } else {
    ImageMemoryDesc& compressed_frame = compressed_frames_[frame_index_].image;
    ImageMemoryDesc& scaled_compressed_frame =
        scaled_compressed_frames_[frame_index_].image;

    const VRect scaled_compressed_frame_rect =
        GetCenteredRectInArea(scale_area, scaled_compressed_frame.width,
                              scaled_compressed_frame.height);
    const VRect cropped_compressed_frame_rect =
        GetCenteredRectInArea(crop_area, cropped_compressed_frame_.width,
                              cropped_compressed_frame_.height);

    if (!DisplayImage(scaled_compressed_frame, scaled_compressed_frame_rect,
                      display_pixels_proc_, &painting_context) ||
        !DisplayImage(cropped_compressed_frame_, cropped_compressed_frame_rect,
                      display_pixels_proc_, &painting_context)) {
      OnError();
      ClearProxyArea();
      return;
    }

    VRect selection_in_scaled_compressed_frame = ScaleRectFromAreaToArea(
        selection_in_compressed_frame_, compressed_frame.width,
        compressed_frame.height, scaled_compressed_frame.width,
        scaled_compressed_frame.height);
    VRect selection_in_window = selection_in_scaled_compressed_frame;
    selection_in_window.left += scaled_compressed_frame_rect.left;
    selection_in_window.right += scaled_compressed_frame_rect.left;
    selection_in_window.top += scaled_compressed_frame_rect.top;
    selection_in_window.bottom += scaled_compressed_frame_rect.top;

    // Selection border in scaled compressed image (left).
    DrawRectBorder(255, 255, 255, selection_in_window.left - 1,
                   selection_in_window.top - 1, selection_in_window.right,
                   selection_in_window.bottom, &painting_context);
    DrawRectBorder(128, 128, 128, selection_in_window.left - 2,
                   selection_in_window.top - 2, selection_in_window.right + 1,
                   selection_in_window.bottom + 1, &painting_context);

    // Selection border in cropped compressed image (right).
    DrawRectBorder(255, 255, 255, cropped_compressed_frame_rect.left - 1,
                   cropped_compressed_frame_rect.top - 1,
                   cropped_compressed_frame_rect.right,
                   cropped_compressed_frame_rect.bottom, &painting_context);
    DrawRectBorder(128, 128, 128, cropped_compressed_frame_rect.left - 2,
                   cropped_compressed_frame_rect.top - 2,
                   cropped_compressed_frame_rect.right + 1,
                   cropped_compressed_frame_rect.bottom + 1, &painting_context);
  }

  EndPainting(&painting_context);
}

//------------------------------------------------------------------------------

void WebPShopDialog::Notify(int32 item) {
  if (item == kDQualitySlider) {
    int quality = quality_slider_.GetValue();
    if (write_config_.quality != quality) {
      write_config_.quality = quality;
      quality_field_.SetValueIfDifferent(quality);
      DiscardEncodedData();
      ForceRepaint();
    }
  } else if (item == kDQualityField) {
    int quality = quality_field_.GetValue();
    if (quality >= 0 && quality <= 100 && write_config_.quality != quality) {
      write_config_.quality = quality;
      quality_slider_.SetValueIfDifferent(quality);
      DiscardEncodedData();
      ForceRepaint();
    }
  } else if (item >= kDCompressionFastest && item <= kDCompressionSlowest) {
    compression_radio_group_.SetSelected(item);
    Compression compression;
    if (item == kDCompressionFastest) {
      compression = Compression::FASTEST;
    } else if (item == kDCompressionDefault) {
      compression = Compression::DEFAULT;
    } else {
      compression = Compression::SLOWEST;
    }
    if (write_config_.compression != compression) {
      write_config_.compression = compression;
      DiscardEncodedData();
      ForceRepaint();
    }
  } else if (item == kDKeepExif) {
    bool keep_exif = keep_exif_checkbox_.GetChecked();
    if (write_config_.keep_exif != keep_exif) {
      write_config_.keep_exif = keep_exif;
      DiscardEncodedData();
      ForceRepaint();
    }
  } else if (item == kDKeepXmp) {
    bool keep_xmp = keep_xmp_checkbox_.GetChecked();
    if (write_config_.keep_xmp != keep_xmp) {
      write_config_.keep_xmp = keep_xmp;
      DiscardEncodedData();
      ForceRepaint();
    }
  } else if (item == kDKeepColorProfile) {
    bool keep_color_profile = keep_color_profile_checkbox_.GetChecked();
    if (write_config_.keep_color_profile != keep_color_profile) {
      write_config_.keep_color_profile = keep_color_profile;
      DiscardEncodedData();
      ForceRepaint();
    }
  } else if (item == kDLoopForever) {
    bool loop_forever = loop_forever_checkbox_.GetChecked();
    if (write_config_.loop_forever != loop_forever) {
      write_config_.loop_forever = loop_forever;
      DiscardEncodedData();
      ForceRepaint();
    }
  } else if (item == kDProxyCheckbox) {
    bool display_proxy = proxy_checkbox_.GetChecked();
    if (write_config_.display_proxy != display_proxy) {
      write_config_.display_proxy = display_proxy;
      ForceRepaint();
    }
  } else if (item == kDFrameSlider) {
    int frame_index = frame_slider_.GetValue();
    if (frame_index >= 0 && frame_index < (int)compressed_frames_.size() &&
        frame_index_ != (size_t)frame_index) {
      frame_index_ = (size_t)frame_index;
      frame_field_.SetValueIfDifferent(frame_index + 1);
      update_cropped_compressed_frame_ = true;
      ForceRepaint();
    }
  } else if (item == kDFrameField) {
    int frame_index = frame_field_.GetValue() - 1;
    if (frame_index >= 0 && frame_index < (int)compressed_frames_.size() &&
        frame_index_ != (size_t)frame_index) {
      frame_index_ = (size_t)frame_index;
      frame_slider_.SetValueIfDifferent(frame_index);
      update_cropped_compressed_frame_ = true;
      ForceRepaint();
    }
  }
}

//------------------------------------------------------------------------------

// Move the selection according to the position of the cursor.
void WebPShopDialog::OnMouseMove(int x, int y, bool left_button_is_held_down) {
  if (!left_button_is_held_down || !write_config_.display_proxy ||
      scaled_compressed_frames_.empty()) {
    return;
  }

  const ImageMemoryDesc& compressed_frame =
      compressed_frames_[frame_index_].image;
  const ImageMemoryDesc& scaled_compressed_frame =
      scaled_compressed_frames_[frame_index_].image;
  const VRect scaled_compressed_frame_rect = GetCenteredRectInArea(
      GetScaleAreaRectInWindow(GetProxyAreaRectInWindow()),
      scaled_compressed_frame.width, scaled_compressed_frame.height);

  if (x >= scaled_compressed_frame_rect.left &&
      x < scaled_compressed_frame_rect.right &&
      y >= scaled_compressed_frame_rect.top &&
      y < scaled_compressed_frame_rect.bottom) {
    x -= scaled_compressed_frame_rect.left;
    y -= scaled_compressed_frame_rect.top;

    VRect selection;
    selection.left =
        (x * compressed_frame.width) / scaled_compressed_frame.width -
        cropped_compressed_frame_.width / 2;
    selection.top =
        (y * compressed_frame.height) / scaled_compressed_frame.height -
        cropped_compressed_frame_.height / 2;

    if (selection.left + cropped_compressed_frame_.width >
        compressed_frame.width) {
      selection.left = compressed_frame.width - cropped_compressed_frame_.width;
    } else if (selection.left < 0) {
      selection.left = 0;
    }
    if (selection.top + cropped_compressed_frame_.height >
        compressed_frame.height) {
      selection.top =
          compressed_frame.height - cropped_compressed_frame_.height;
    } else if (selection.top < 0) {
      selection.top = 0;
    }

    selection.right = selection.left + cropped_compressed_frame_.width;
    selection.bottom = selection.top + cropped_compressed_frame_.height;

    if (selection_in_compressed_frame_.left != selection.left ||
        selection_in_compressed_frame_.top != selection.right) {
      selection_in_compressed_frame_ = selection;
      update_cropped_compressed_frame_ = true;
      ForceRepaint();
    }
  }
}
