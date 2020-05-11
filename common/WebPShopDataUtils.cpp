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

#include <fstream>
#include <string>

#include "FileUtilities.h"
#include "WebPShop.h"

//------------------------------------------------------------------------------

void ReadSome(size_t count, void* const buffer, FormatRecordPtr format_record,
              int16* const result) {
  if (*result != noErr) return;
  int32 read_count = (int32)count;
  *result = PSSDKRead((int32)format_record->dataFork,
                      format_record->posixFileDescriptor,
                      format_record->pluginUsingPOSIXIO, &read_count, buffer);
  if (*result == noErr && (size_t)read_count != count) *result = eofErr;
  if (*result != noErr) LOG("/!\\ Unable to read " << count << " bytes.");
}
void WriteSome(size_t count, const void* const buffer,
               FormatRecordPtr format_record, int16* const result) {
  if (*result != noErr) return;
  int32 write_count = (int32)count;
  *result = PSSDKWrite(
      (int32)format_record->dataFork, format_record->posixFileDescriptor,
      format_record->pluginUsingPOSIXIO, &write_count, (void*)buffer);
  if (*result == noErr && (size_t)write_count != count) *result = dskFulErr;
  if (*result != noErr) LOG("/!\\ Unable to write " << count << " bytes.");
}

void Allocate(size_t count, void** const buffer, int16* const result) {
  if (buffer == nullptr) {
    *result = paramErr;
    return;
  }
  unsigned32 buffer_count = (unsigned32)count;
  *buffer = sPSBuffer->New(&buffer_count, (unsigned32)count);
  if (*buffer == nullptr || buffer_count != (unsigned32)count) {
    Deallocate(buffer);
    *result = memFullErr;
  }
  if (*result != noErr) LOG("/!\\ Unable to allocate " << count << " bytes.");
}
void AllocateAndRead(size_t count, void** const buffer,
                     FormatRecordPtr format_record, int16* const result) {
  Allocate(count, buffer, result);
  if (*result != noErr) return;

  *result = PSSDKSetFPos((int32)format_record->dataFork,
                         format_record->posixFileDescriptor,
                         format_record->pluginUsingPOSIXIO, fsFromStart, 0);
  if (*result != noErr) {
    LOG("/!\\ Unable to set cursor at the beginning of the file.");
    Deallocate(buffer);
    return;
  }

  ReadSome(count, *buffer, format_record, result);
  if (*result != noErr) {
    Deallocate(buffer);
    return;
  }
}
void Deallocate(void** const buffer) {
  if (buffer == nullptr) return;
  Ptr ptr = (Ptr)*buffer;
  *buffer = nullptr;
  sPSBuffer->Dispose(&ptr);
}

//------------------------------------------------------------------------------

bool ReadAndCheckHeader(FormatRecordPtr format_record, int16* const result,
                        size_t* file_size) {
  if (*result != noErr) return false;

  *result = PSSDKSetFPos((int32)format_record->dataFork,
                         format_record->posixFileDescriptor,
                         format_record->pluginUsingPOSIXIO, fsFromStart, 0);
  if (*result != noErr) return false;

  uint8_t file_header[12];
  ReadSome(sizeof(file_header), file_header, format_record, result);
  if (*result != noErr) return false;

  if (memcmp(file_header, "RIFF", 4) != 0 ||
      memcmp(file_header + 8, "WEBP", 4) != 0) {
    *result = formatCannotRead;
    return false;
  }

  if (file_size != nullptr) {
    *file_size = ((size_t)(file_header[4] << 0) | (file_header[5] << 8) |
                  (file_header[6] << 16) | (file_header[7] << 24)) +
                 8;
    LOG("File size: " << *file_size);
  }
  return true;
}
