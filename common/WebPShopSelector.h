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

#ifndef __WebPShopSelector_H__
#define __WebPShopSelector_H__

#include "PIFormat.h"
#include "WebPShop.h"

//------------------------------------------------------------------------------

// When the Open menu command is selected, DoReadPrepare() and DoReadStart()
// will be called once, followed by DoReadContinue() as long as the host did not
// receive the entire image (by chunks defined by format_record->theRect32).
// DoReadFinish() is then called once for cleanup.
// data->file_data contains input data and format_record->data must be filled
// with decoded pixels, then null when it's over.
void DoReadPrepare(FormatRecordPtr format_record, Data* const data,
                   int16* const result);
void DoReadStart(FormatRecordPtr format_record, Data* const data,
                 int16* const result);
void DoReadContinue(FormatRecordPtr format_record, Data* const data,
                    int16* const result);
void DoReadFinish(FormatRecordPtr format_record, Data* const data,
                  int16* const result);

// If FormatLayerSupport contains doesSupportFormatLayers in WebPShop.r,
// DoReadContinue() is no longer called if format_record->layerData > 0;
// it is replaced by DoReadLayerStart(), then multiple DoReadLayerContinue(),
// then DoReadLayerFinish(). These three functions are called in this order as
// many times as specified in format_record->layerData by the plugin.
void DoReadLayerStart(FormatRecordPtr format_record, Data* const data,
                      int16* const result);
void DoReadLayerContinue(FormatRecordPtr format_record, Data* const data,
                         int16* const result);
void DoReadLayerFinish(FormatRecordPtr format_record, Data* const data,
                       int16* const result);

// When the Save menu command is selected, DoOptions*() are called.
// It is time to show the user a dialog with encoding settings.
// If a preview was shown, the encoded_data is reused for the writing step.
void DoOptionsPrepare(FormatRecordPtr format_record, Data* const data,
                      int16* const result);
void DoOptionsStart(FormatRecordPtr format_record, Data* const data,
                    int16* const result, const SPPluginRef& plugin_ref);
void DoOptionsContinue(FormatRecordPtr format_record, Data* const data,
                       int16* const result);
void DoOptionsFinish(FormatRecordPtr format_record, Data* const data,
                     int16* const result);

// Then an estimation of the output file size is requested by the host.
// It must be set in format_record->minDataBytes and maxDataBytes.
void DoEstimatePrepare(FormatRecordPtr format_record, Data* const data,
                       int16* const result);
void DoEstimateStart(FormatRecordPtr format_record, Data* const data,
                     int16* const result);
void DoEstimateContinue(FormatRecordPtr format_record, Data* const data,
                        int16* const result);
void DoEstimateFinish(FormatRecordPtr format_record, Data* const data,
                      int16* const result);

// Then we write the encoded data to the file opened by the host.
// DoWriteContinue() is called until the whole canvas was requested to the
// host (by chunks defined by format_record->theRect32).
// Input data is in format_record->data, and it must be set to null when it's
// over.
// Here format_record->data is not used; data is directly copied through
// CopyWholeCanvas() and CopyAllLayers(), as used for the UI preview.
void DoWritePrepare(FormatRecordPtr format_record, Data* const data,
                    int16* const result);
void DoWriteStart(FormatRecordPtr format_record, Data* const data,
                  int16* const result);
void DoWriteContinue(FormatRecordPtr format_record, Data* const data,
                     int16* const result);
void DoWriteFinish(FormatRecordPtr format_record, Data* const data,
                   int16* const result);

// If FormatLayerSupport is in WebPShop.r but not FormatLayerSupportReadOnly,
// DoWriteContinue() is no longer called; it is replaced by DoWriteLayerStart(),
// then multiple DoWriteLayerContinue(), then DoWriteLayerFinish(). These three
// functions are called in this order as many times as specified in
// format_record->layerData by the host.
void DoWriteLayerStart(FormatRecordPtr format_record, Data* const data,
                       int16* const result);
void DoWriteLayerContinue(FormatRecordPtr format_record, Data* const data,
                          int16* const result);
void DoWriteLayerFinish(FormatRecordPtr format_record, Data* const data,
                        int16* const result);

// It's called by the host to know which plugins can open the file,
// only if several installed plugins handle .webp files.
void DoFilterFile(FormatRecordPtr format_record, Data* const data,
                  int16* const result);

//------------------------------------------------------------------------------

#endif  // __WebPShopSelector_H__
