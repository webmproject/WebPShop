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

#include <exception>

#include "PIFormat.h"
#include "WebPShopSelector.h"

DLLExport MACPASCAL void PluginMain(const int16 selector,
                                    FormatRecordPtr formatParamBlock,
                                    intptr_t* dataHandle, int16* result);

SPBasicSuite* sSPBasic = NULL;  // External reference for resource allocation.

static bool TryAcquireSuite(Data* const data, int16* const result) {
  if (sSPBasic->AcquireSuite(kPSChannelPortsSuite, kPSChannelPortsSuiteVersion3,
                             (const void**)&data->sPSChannelPortsSuite) ||
      data->sPSChannelPortsSuite == nullptr) {
    LOG("/!\\ Unable to acquire suite.");
    *result = errPlugInHostInsufficient;
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
//
// PluginMain / main (description from Adobe SDK)
//
// All calls to the plug-in module come through this routine. It must be placed
// first in the resource. To achieve this, most development systems require this
// be the first routine in the source.
//
// The entrypoint will be "pascal void" for Macintosh, "void" for Windows.
//
// Parameters:
//   const int16 selector
//     Host provides selector indicating what command to do.
//   FormatRecordPtr formatParamBlock
//     Host provides pointer to parameter block containing pertinent data and
//     callbacks from the host. See PIFormat.h.
//   intptr_t* dataPointer
//     Use this to store a pointer to our global parameters structure,
//     which is maintained by the host between calls to the plug-in.
//   int16* result
//     Return error result or noErr. Some errors are handled by the host, some
//     are silent, and some you must handle. See PIGeneral.h.
//
//------------------------------------------------------------------------------

DLLExport MACPASCAL void PluginMain(const int16 selector,
                                    FormatRecordPtr formatParamBlock,
                                    intptr_t* dataPointer, int16* result) {
  try {
    // The about box is a special request; the parameter block is not filled
    // out, none of the callbacks or standard data is available. Instead, the
    // parameter block points to an AboutRecord, which is used on Windows.
    if (selector == formatSelectorAbout) {
      AboutRecordPtr aboutRecord =
          reinterpret_cast<AboutRecordPtr>(formatParamBlock);
      sSPBasic = aboutRecord->sSPBasic;
      SPPluginRef plugin_ref =
          reinterpret_cast<SPPluginRef>(aboutRecord->plugInRef);
      DoAboutBox(plugin_ref);
    } else {
      FormatRecordPtr format_record = formatParamBlock;
      sSPBasic = format_record->sSPBasic;
      SPPluginRef plugin_ref =
          reinterpret_cast<SPPluginRef>(format_record->plugInRef);

      // For Photoshop 8, big documents, rows and columns are now > 30000 px.
      if (format_record->HostSupports32BitCoordinates) {
        format_record->PluginUsing32BitCoordinates = true;
      }

      // Allocate and initialize data.
      Data* data = (Data*)*dataPointer;
      if (data == nullptr) {
        LOG(std::endl << "Initialization.");
        data = (Data*)malloc(sizeof(Data));
        if (data == nullptr) {
          LOG("/!\\ Unable to allocate " << sizeof(Data) << " bytes.");
          *result = memFullErr;
          return;
        }

        // Photoshop should take care of freeing it.
        *dataPointer = (intptr_t)data;

        // Init data.
        WebPInitDecoderConfig(&data->read_config);
        data->write_config.quality = 75;
        data->write_config.compression = Compression::DEFAULT;
        data->write_config.keep_exif = false;
        data->write_config.keep_xmp = false;
        data->write_config.keep_color_profile = false;
        data->write_config.loop_forever = true;
        data->write_config.animation = false;
        data->write_config.display_proxy = false;
        data->file_size = 0;
        data->file_data = nullptr;
        data->metadata[Metadata::kEXIF].four_cc = "EXIF";
        data->metadata[Metadata::kXMP].four_cc = "XMP ";
        data->metadata[Metadata::kICCP].four_cc = "ICCP";
        for (Metadata& metadata : data->metadata) WebPDataInit(&metadata.chunk);
        WebPDataInit(&data->encoded_data);
        data->anim_decoder = nullptr;
        data->last_frame_timestamp = 0;
        data->sPSChannelPortsSuite = nullptr;

        if (!TryAcquireSuite(data, result)) return;
      }

      // Dispatch selector.
      switch (selector) {
        case formatSelectorReadPrepare:
          DoReadPrepare(format_record, data, result);
          break;
        case formatSelectorReadStart:
          DoReadStart(format_record, data, result);
          break;
        case formatSelectorReadContinue:
          DoReadContinue(format_record, data, result);
          break;
        case formatSelectorReadFinish:
          DoReadFinish(format_record, data, result);
          break;

        case formatSelectorOptionsPrepare:
          DoOptionsPrepare(format_record, data, result);
          break;
        case formatSelectorOptionsStart:
          DoOptionsStart(format_record, data, result, plugin_ref);
          break;
        case formatSelectorOptionsContinue:
          DoOptionsContinue(format_record, data, result);
          break;
        case formatSelectorOptionsFinish:
          DoOptionsFinish(format_record, data, result);
          break;

        case formatSelectorEstimatePrepare:
          DoEstimatePrepare(format_record, data, result);
          break;
        case formatSelectorEstimateStart:
          DoEstimateStart(format_record, data, result);
          break;
        case formatSelectorEstimateContinue:
          DoEstimateContinue(format_record, data, result);
          break;
        case formatSelectorEstimateFinish:
          DoEstimateFinish(format_record, data, result);
          break;

        case formatSelectorWritePrepare:
          DoWritePrepare(format_record, data, result);
          break;
        case formatSelectorWriteStart:
          DoWriteStart(format_record, data, result);
          break;
        case formatSelectorWriteContinue:
          DoWriteContinue(format_record, data, result);
          break;
        case formatSelectorWriteFinish:
          DoWriteFinish(format_record, data, result);
          break;

        case formatSelectorReadLayerStart:
          DoReadLayerStart(format_record, data, result);
          break;
        case formatSelectorReadLayerContinue:
          DoReadLayerContinue(format_record, data, result);
          break;
        case formatSelectorReadLayerFinish:
          DoReadLayerFinish(format_record, data, result);
          break;

        case formatSelectorWriteLayerStart:
          DoWriteLayerStart(format_record, data, result);
          break;
        case formatSelectorWriteLayerContinue:
          DoWriteLayerContinue(format_record, data, result);
          break;
        case formatSelectorWriteLayerFinish:
          DoWriteLayerFinish(format_record, data, result);
          break;

        case formatSelectorFilterFile:
          DoFilterFile(format_record, data, result);
          break;
      }
    }

    // Release any suites that we may have acquired.
    if (selector == formatSelectorAbout ||
        selector == formatSelectorWriteFinish ||
        selector == formatSelectorReadFinish ||
        selector == formatSelectorOptionsFinish ||
        selector == formatSelectorEstimateFinish ||
        selector == formatSelectorFilterFile || *result != noErr) {
      PIUSuitesRelease();
    }

  } catch (const std::exception& e) {
    (void)e;
    LOG("/!\\ Exception: " << e.what());
    if (result != nullptr) *result = -1;
  } catch (...) {
    LOG("/!\\ Caught an unknown exception.");
    if (result != nullptr) *result = -1;
  }
}
