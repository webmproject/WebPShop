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

#include <time.h>

#include <string>

#include "PIUSuites.h"
#include "WebPShop.h"

//------------------------------------------------------------------------------

void AddComment(FormatRecordPtr format_record, Data* const data,
                int16* const result) {
  time_t ltime;
  time(&ltime);

  const std::string currentTime = ctime(&ltime);

  size_t length = currentTime.size();

  Handle h = sPSHandle->New((int32)length);

  if (h != NULL) {
    Boolean oldLock = FALSE;
    Ptr p = NULL;
    sPSHandle->SetLock(h, true, &p, &oldLock);
    if (p != NULL) {
      for (size_t a = 0; a < length; a++) *p++ = currentTime.at(a);
      format_record->resourceProcs->addProc(histResource, h);
      sPSHandle->SetLock(h, false, &p, &oldLock);
    }
    sPSHandle->Dispose(h);
  }
}
