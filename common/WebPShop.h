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

// WebPShop is a Photoshop plugin for opening and saving WebP images and
// animations.
// Terms used:
//   Host:       Photoshop
//   Module:     plugin
//   Format:     used with Open, Open As, Save, Save As commands
//   Selector:   host asking for an action
//   UI:         related to the window displaying encoding settings
//   Proxy:      UI preview of the compressed image, to see the quality
//   Scale area: if the image is too big to fit in the proxy area, it is
//               rescaled to fit in the scale area (left), and a selected
//               cropped subimage is displayed (right).

#ifndef __WebPShop_H__
#define __WebPShop_H__

#include <fstream>
#include <string>
#include <vector>

#include "PIFormat.h"
#include "PITypes.h"
#include "PIUtilities.h"
#include "WebPShopTerminology.h"
#include "webp/decode.h"
#include "webp/demux.h"
#include "webp/encode.h"

//------------------------------------------------------------------------------
// Settings

// Uncomment to enable logging to a local file.
// #define LOG_TO_FILE "C:/logs/webpshop_log.txt"

// Uncomment to log the duration of some tasks (requires LOG_TO_FILE).
// #define MEASURE_TIME

#define MAX_NUM_BROWSED_CHANNELS 16
#define MAX_NUM_BROWSED_LAYERS 4096

//------------------------------------------------------------------------------
// Macros

#ifdef LOG_TO_FILE
#define LOG(STR)                                   \
  do {                                             \
    std::ofstream log(LOG_TO_FILE, std::ios::app); \
    log << STR << std::endl;                       \
    log.close();                                   \
  } while (0)
#else
#define LOG(STR) do { } while (0)
#endif  // LOG_TO_FILE

#ifdef MEASURE_TIME
#include <chrono>
#define START_TIMER(NAME)                              \
  std::chrono::steady_clock::time_point begin_##NAME = \
      std::chrono::steady_clock::now()
#define STOP_TIMER(NAME)                                         \
  LOG(#NAME " took "                                             \
      << (std::chrono::duration_cast<std::chrono::microseconds>( \
              std::chrono::steady_clock::now() - begin_##NAME)   \
              .count() /                                         \
          1000000.0)                                             \
      << " seconds.")
#else
#define START_TIMER(NAME) do { } while (0)
#define STOP_TIMER(NAME) do { } while (0)
#endif  // MEASURE_TIME

//------------------------------------------------------------------------------
// Data

enum Compression { FASTEST = 0, DEFAULT = 1, SLOWEST = 2 };

// Encoding parameters, closely tied to the UI.
struct WriteConfig {
  int quality;  // [0..100]
  Compression compression;
  bool keep_exif;
  bool keep_xmp;
  bool keep_color_profile;
  bool loop_forever;
  bool animation;
  bool display_proxy;
};

struct Metadata {
  enum kType { kEXIF, kXMP, kICCP, kNum };
  const char* four_cc;
  WebPData chunk;
};

// An instance of Data will be allocated on the first time the plugin is
// solicited and it will be freed by Photoshop. Everything that must stay
// between plugin calls should go into it (to avoid globals).
struct Data {
  WebPDecoderConfig read_config;
  WriteConfig write_config;
  size_t file_size;
  void* file_data;
  Metadata metadata[Metadata::kNum];
  WebPData encoded_data;
  WebPAnimDecoder* anim_decoder;
  int last_frame_timestamp;
  uint16 layer_name_buffer[256];
  PSChannelPortsSuite1* sPSChannelPortsSuite;
};

// Stores an image.
struct ImageMemoryDesc {
  PixelMemoryDesc pixels = {nullptr, 0, 0, 0, 0};
  int32 width = 0;
  int32 height = 0;
  int num_channels = 0;
  int32 mode = 0;
};

// Stores a frame (image with a duration).
struct FrameMemoryDesc {
  ImageMemoryDesc image;
  int duration_ms = 0;
};

//------------------------------------------------------------------------------
// User interface

// Displays a window with writing (encoding) options.
bool DoUI(WriteConfig* const write_config,
          const Metadata metadata[Metadata::kNum], SPPluginRef plugin_ref,
          const std::vector<FrameMemoryDesc>& original_frames,
          bool original_frames_were_converted_to_8b,
          WebPData* const encoded_data, DisplayPixelsProc display_pixels_proc);
void DoAboutBox(SPPluginRef plugin_ref);

// Loads and saves scripting parameters.
void LoadWriteConfig(FormatRecordPtr format_record,
                     WriteConfig* const write_config, int16* const result);
void SaveWriteConfig(FormatRecordPtr format_record,
                     const WriteConfig& write_config, int16* const result);

void LoadPOSIXConfig(FormatRecordPtr format_record, int16* const result);

//------------------------------------------------------------------------------
// Utils

// This routine adds a history entry with the current system date and time
// to the file opened by host.
void AddComment(FormatRecordPtr format_record, Data* const data,
                int16* const result);

//------------------------------------------------------------------------------
// Data utils

// Reads from and writes to file opened by host.
void ReadSome(size_t count, void* const buffer, FormatRecordPtr format_record,
              int16* const result);
void WriteSome(size_t count, const void* const buffer,
               FormatRecordPtr format_record, int16* const result);

// Checks that first bytes are "RIFF" then file size then "WEBP".
bool ReadAndCheckHeader(FormatRecordPtr format_record, int16* const result,
                        size_t* file_size);

// Memory managed by host.
void Allocate(size_t count, void** const buffer, int16* const result);
void AllocateAndRead(size_t count, void** const buffer,
                     FormatRecordPtr format_record, int16* const result);
void Deallocate(void** const buffer);

//------------------------------------------------------------------------------
// Image utils

bool AllocateImage(ImageMemoryDesc* const image, int32 width, int32 height,
                   int num_channels, int32 bit_depth);
void DeallocateImage(ImageMemoryDesc* const image);
void DeallocateMetadata(Metadata metadata[Metadata::kNum]);

// Deallocates frames that won't be used after vector::resize().
void ResizeFrameVector(std::vector<FrameMemoryDesc>* const frames, size_t size);
void ClearFrameVector(std::vector<FrameMemoryDesc>* const frames);

bool Scale(const ImageMemoryDesc& src, ImageMemoryDesc* const dst,
           size_t dst_width, size_t dst_height);
bool Crop(const ImageMemoryDesc& src, ImageMemoryDesc* const dst,
          size_t crop_width, size_t crop_height, size_t crop_left,
          size_t crop_top);
bool To8bit(const ImageMemoryDesc& src, bool add_alpha,
            ImageMemoryDesc* const dst);

//------------------------------------------------------------------------------
// Dimensions utils

int32 GetWidth(const VRect& rect);
int32 GetHeight(const VRect& rect);
bool ScaleToFit(int32* const width, int32* const height, int32 max_width,
                int32 max_height);
bool CropToFit(int32* const width, int32* const height, int32 left, int32 top,
               int32 max_width, int32 max_height);
VRect ScaleRectFromAreaToArea(const VRect& src, int32 src_area_width,
                              int32 src_area_height, int32 dst_area_width,
                              int32 dst_area_height);

//------------------------------------------------------------------------------
// Canvas utils

// Requests data for whole canvas from host, stores it in format_record->data.
void RequestWholeCanvas(FormatRecordPtr format_record, int16* const result);
// Copies merged canvas from host into destination.
void CopyWholeCanvas(FormatRecordPtr format_record, Data* const data,
                     int16* const result, ImageMemoryDesc* const destination);
// Copies each layer (without effects) from host into destination.
void CopyAllLayers(FormatRecordPtr format_record, Data* const data,
                   int16* const result,
                   std::vector<FrameMemoryDesc>* const destination);

//------------------------------------------------------------------------------
// Encode utils

// Fills config parameters with settings in write_config.
void SetWebPConfig(WebPConfig* const config, const WriteConfig& write_config);

// Wraps an ImageMemoryDesc into a WebPPicture.
// WebPPictureInit() must be called on 'dst' prior to calling this and
// WebPPictureFree() must be called afterwards.
bool CastToWebPPicture(const WebPConfig& config, const ImageMemoryDesc& src,
                       WebPPicture* const dst);

// Encodes original_image into encoded_data.
bool EncodeOneImage(const ImageMemoryDesc& original_image,
                    const WriteConfig& write_config,
                    WebPData* const encoded_data);
bool EncodeAllFrames(const std::vector<FrameMemoryDesc>& original_frames,
                     const WriteConfig& write_config,
                     WebPData* const encoded_data);

// Retrieves metadata from host (current Photoshop document).
OSErr GetHostMetadata(FormatRecordPtr format_record,
                      Metadata metadata[Metadata::kNum]);
// Adds kept available metadata (EXIF, XMP, ICC) to encoded_data.
bool EncodeMetadata(const WriteConfig& write_config,
                    const Metadata metadata[Metadata::kNum],
                    WebPData* const encoded_data);

// Writes encoded_data to file opened by host.
void WriteToFile(const WebPData& encoded_data, FormatRecordPtr format_record,
                 int16* const result);

//------------------------------------------------------------------------------
// Decode utils

// Decodes encoded_data into compressed_image.
bool DecodeOneImage(const WebPData& encoded_data,
                    ImageMemoryDesc* const compressed_image);
bool DecodeAllFrames(const WebPData& encoded_data,
                     std::vector<FrameMemoryDesc>* const compressed_frames);

// Gets available metadata from encoded_data.
bool DecodeMetadata(const WebPData& encoded_data,
                    Metadata metadata[Metadata::kNum]);
// Sends metadata to host (current Photoshop document).
OSErr SetHostMetadata(FormatRecordPtr format_record,
                      const Metadata metadata[Metadata::kNum]);

// Decodes file opened by host into format_record->data.
void InitAnimDecoder(FormatRecordPtr format_record, Data* const data,
                     int16* const result);
void ReadOneFrame(FormatRecordPtr format_record, Data* const data,
                  int16* const result, int frame_counter);
void ReleaseAnimDecoder(FormatRecordPtr format_record, Data* const data);

// Sets FormatRecord members that should be defined in both the start and
// continue selectors, according to PIFormat.h.
void SetPlaneColRowBytes(FormatRecordPtr format_record);

//------------------------------------------------------------------------------
// Animation utils

// Prints "Frame [frame_index + 1] ([frame_duration] ms)" to output.
void PrintDuration(int frame_index, int frame_duration, uint16 output[],
                   size_t output_max_length);
// Extracts a number from last encountered frame duration syntax "(123 ms)".
bool TryExtractDuration(const uint16* const layer_name, int* const duration_ms);

//------------------------------------------------------------------------------

#endif  // __WebPShop_H__
