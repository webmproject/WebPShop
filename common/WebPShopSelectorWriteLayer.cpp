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

#include "PIFormat.h"
#include "WebPShop.h"
#include "WebPShopSelector.h"

//------------------------------------------------------------------------------

void DoWriteLayerStart(FormatRecordPtr format_record, Data* const data,
                       int16* const result) {}

//------------------------------------------------------------------------------

void DoWriteLayerContinue(FormatRecordPtr format_record, Data* const data,
                          int16* const result) {
  Deallocate(&format_record->data);
}

//------------------------------------------------------------------------------

void DoWriteLayerFinish(FormatRecordPtr format_record, Data* const data,
                        int16* const result) {}
