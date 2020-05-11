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

#include "PIActions.h"
#include "PIUSuites.h"
#include "WebPShop.h"

static void LoadScriptingParameters(FormatRecordPtr format_record,
                                    WriteConfig* const write_config,
                                    bool* const use_posix,
                                    int16* const result) {
  DescriptorKeyID key = 0;
  DescriptorTypeID type = 0;
  DescriptorKeyIDArray array = {NULLID};
  int32 flags = 0;

  PIDescriptorParameters* descParams = format_record->descriptorParameters;
  if (descParams == nullptr) {
    LOG("/!\\ Reading parameters: No descriptorParameters.");
    return;
  }

  ReadDescriptorProcs* readProcs =
      format_record->descriptorParameters->readDescriptorProcs;
  if (readProcs == nullptr) {
    LOG("/!\\ Reading parameters: No readDescriptorProcs.");
    return;
  }
  if (descParams->descriptor == nullptr) {
    LOG("Reading parameters: No descriptor (no stored parameter).");
    return;
  }

  PIReadDescriptor token =
      readProcs->openReadDescriptorProc(descParams->descriptor, array);
  if (token == nullptr) {
    LOG("/!\\ Reading parameters: No token.");
    return;
  }

  while (readProcs->getKeyProc(token, &key, &type, &flags)) {
    switch (key) {
      case keyWriteConfig_quality: {
        int32 i;
        readProcs->getIntegerProc(token, &i);
        if (i < 0 || i > 100) {
          LOG("/!\\ Reading parameters: Out of bounds.");
        } else if (write_config != nullptr) {
          write_config->quality = i;
        }
        LOG("Reading parameter: quality = " << i);
        break;
      }
      case keyWriteConfig_compression: {
        int32 i;
        readProcs->getIntegerProc(token, &i);
        if (i < (int32)Compression::FASTEST ||
            i > (int32)Compression::SLOWEST) {
          LOG("/!\\ Reading parameters: Out of bounds.");
        } else if (write_config != nullptr) {
          write_config->compression = (Compression)i;
        }
        LOG("Reading parameter: compression = " << i);
        break;
      }
      case keyWriteConfig_keep_exif: {
        Boolean b;
        readProcs->getBooleanProc(token, &b);
        if (write_config != nullptr) write_config->keep_exif = (bool)b;
        LOG("Reading parameter: exif = " << (bool)b);
        break;
      }
      case keyWriteConfig_keep_xmp: {
        Boolean b;
        readProcs->getBooleanProc(token, &b);
        if (write_config != nullptr) write_config->keep_xmp = (bool)b;
        LOG("Reading parameter: xmp = " << (bool)b);
        break;
      }
      case keyWriteConfig_keep_color_profile: {
        Boolean b;
        readProcs->getBooleanProc(token, &b);
        if (write_config != nullptr) write_config->keep_color_profile = (bool)b;
        LOG("Reading parameter: color profile = " << (bool)b);
        break;
      }
      case keyWriteConfig_loop_forever: {
        Boolean b;
        readProcs->getBooleanProc(token, &b);
        if (write_config != nullptr) write_config->loop_forever = (bool)b;
        LOG("Reading parameter: loop forever = " << (bool)b);
        break;
      }
      case keyUsePOSIX: {
        Boolean b;
        readProcs->getBooleanProc(token, &b);
        if (use_posix != nullptr) *use_posix = (bool)b;
        LOG("Reading parameter: posix = " << (bool)b);
        break;
      }
      default: {
        LOG("Ignoring parameter key: " << key);
        break;
      }
    }
  }

  *result = readProcs->closeReadDescriptorProc(token);
  sPSHandle->Dispose(descParams->descriptor);
  descParams->descriptor = nullptr;

  if (*result != noErr) {
    LOG("/!\\ Reading parameters: Error " << *result);
  }
}

void LoadWriteConfig(FormatRecordPtr format_record,
                     WriteConfig* const write_config, int16* const result) {
  LoadScriptingParameters(format_record, write_config, nullptr, result);
}

void SaveWriteConfig(FormatRecordPtr format_record,
                     const WriteConfig& write_config, int16* const result) {
  PIDescriptorParameters* descParams = format_record->descriptorParameters;
  if (descParams == NULL) {
    LOG("/!\\ Writing parameters: No descriptorParameters.");
    return;
  }

  WriteDescriptorProcs* writeProcs = descParams->writeDescriptorProcs;
  if (writeProcs == NULL) {
    LOG("/!\\ Writing parameters: No writeDescriptorProcs.");
    return;
  }

  PIWriteDescriptor token = writeProcs->openWriteDescriptorProc();
  if (token == NULL) {
    LOG("/!\\ Writing parameters: No token.");
    return;
  }

  LOG("Writing parameters: quality = " << write_config.quality);
  LOG("                    compression = " << write_config.compression);
  LOG("                    keep "
      << (write_config.keep_exif ? "EXIF" : "NO EXIF") << ", "
      << (write_config.keep_xmp ? "XMP" : "NO XMP") << ", "
      << (write_config.keep_color_profile ? "ICC" : "NO ICC"));
  LOG("                    loop forever = "
      << (write_config.loop_forever ? "yes" : "no"));

  writeProcs->putIntegerProc(token, keyWriteConfig_quality,
                             write_config.quality);
  writeProcs->putIntegerProc(token, keyWriteConfig_compression,
                             write_config.compression);
  writeProcs->putBooleanProc(token, keyWriteConfig_keep_exif,
                             write_config.keep_exif);
  writeProcs->putBooleanProc(token, keyWriteConfig_keep_xmp,
                             write_config.keep_xmp);
  writeProcs->putBooleanProc(token, keyWriteConfig_keep_color_profile,
                             write_config.keep_color_profile);
  writeProcs->putBooleanProc(token, keyWriteConfig_loop_forever,
                             write_config.loop_forever);

  sPSHandle->Dispose(descParams->descriptor);
  PIDescriptorHandle h;
  *result = writeProcs->closeWriteDescriptorProc(token, &h);
  descParams->descriptor = h;

  if (*result != noErr) {
    LOG("/!\\ Writing parameters: Error " << *result);
  }
}

void LoadPOSIXConfig(FormatRecordPtr format_record, int16* const result) {
  bool use_posix;
  LoadScriptingParameters(format_record, nullptr, &use_posix, result);
  format_record->pluginUsingPOSIXIO = format_record->hostSupportsPOSIXIO;
}
