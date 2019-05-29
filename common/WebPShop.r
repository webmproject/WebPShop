// # Copyright 2018 Google LLC
// #
// # Licensed under the Apache License, Version 2.0 (the "License");
// # you may not use this file except in compliance with the License.
// # You may obtain a copy of the License at
// #
// #     https://www.apache.org/licenses/LICENSE-2.0
// #
// # Unless required by applicable law or agreed to in writing, software
// # distributed under the License is distributed on an "AS IS" BASIS,
// # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// # See the License for the specific language governing permissions and
// # limitations under the License.
// (Symbol # is used in case this file is wrongly recognized as R language)

//------------------------------------------------------------------------------
// Definitions -- Required by include files.
//------------------------------------------------------------------------------

#define plugInName "WebP"
#define plugInCopyrightYear "2019"
#define plugInDescription \
  "A Photoshop plug-in for reading and writing WebP files."

//------------------------------------------------------------------------------
// Definitions -- Required by other resources in this resources file.
//------------------------------------------------------------------------------

// Dictionary (aete) resources:

#define vendorName "Google"
#define plugInAETEComment "webpshop webp"

#define plugInSuiteID 'sdK4'
#define plugInClassID 'simP'
#define plugInEventID typeNull  // must be this

//------------------------------------------------------------------------------
// Set up included files for Macintosh and Windows.
//------------------------------------------------------------------------------

#include "PIDefines.h"

#if __PIMac__
#include "PIGeneral.r"
#include "PIUtilities.r"
#elif defined(__PIWin__)
#define Rez
#include "PIGeneral.h"
#include "PIUtilities.r"
#endif

#include "PIActions.h"
#include "PITerminology.h"

#include "WebPShopTerminology.h"

//------------------------------------------------------------------------------
// PiPL resource
//------------------------------------------------------------------------------

resource 'PiPL'(ResourceID, plugInName " PiPL", purgeable){
    {Kind{ImageFormat}, Name{plugInName},
     Version{(latestFormatVersion << 16) | latestFormatSubVersion},

     Component{ComponentNumber, plugInName},

#ifdef __PIMac__
     CodeMacIntel64{"PluginMain"},
#else
#if defined(_WIN64)
     CodeWin64X86{"PluginMain"},
#else
     CodeWin32X86{"PluginMain"},
#endif
#endif

     // This plug-in can read and write via POSIX I/O routines
     SupportsPOSIXIO{},

     // ClassID, eventID, aete ID, uniqueString:
     HasTerminology{plugInClassID, plugInEventID, ResourceID,
                    vendorName " " plugInName},

     SupportedModes{noBitmap, noGrayScale, noIndexedColor, doesSupportRGBColor,
                    noCMYKColor, noHSLColor, noHSBColor, noMultichannel,
                    noDuotone, noLABColor},
     EnableInfo{"in (PSHOP_ImageMode, RGBMode)"},

     // WebP is limited to 16383 in width and height.
     PlugInMaxSize{16383, 16383}, FormatMaxSize{{16383, 16383}},

     FormatMaxChannels{{1, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24}},

     FmtFileType{'WEBP', '8BIM'},
     // ReadTypes { { '8B1F', '    ' } },
     FilteredTypes{{'8B1F', '    '}}, ReadExtensions{{'WEBP'}},
     WriteExtensions{{'WEBP'}}, FilteredExtensions{{'WEBP'}},
     FormatFlags{fmtSavesImageResources, fmtCanRead, fmtCanWrite,
                 fmtCanWriteIfRead, fmtCanWriteTransparency,
                 fmtCanCreateThumbnail},
     FormatICCFlags{iccCannotEmbedGray, iccCannotEmbedIndexed,
                    iccCannotEmbedRGB, iccCannotEmbedCMYK},

     // Layer support is needed for reading animated WebPs into layers
     // (also it would be better to also load it into the timeline).
     FormatLayerSupport{doesSupportFormatLayers},
     // However it is not needed for writing because layer data is directly
     // fetched with CopyAllLayers() thus there is no need to wait for
     // Photoshop and DoWriteLayer*(). Also with FormatLayerSupportReadOnly
     // Ctrl+S defaults to .psd instead of .webp, which is the default
     // behavior for .png etc.
     FormatLayerSupportReadOnly{}}};

//------------------------------------------------------------------------------
// Dictionary (scripting) resource
//------------------------------------------------------------------------------

resource 'aete'(ResourceID, plugInName " dictionary", purgeable){
    1,
    0,
    english,
    roman, /* aete version and language specifiers */
    {
        vendorName,            /* vendor suite name */
        "WebP format plug-in", /* optional description */
        plugInSuiteID,         /* suite ID */
        1,                     /* suite code, must be 1 */
        1,                     /* suite level, must be 1 */
        {},                    /* structure for filters */
        {
            /* non-filter plug-in class here */
            vendorName " WebP format", /* unique class name */
            plugInClassID,          /* class ID, must be unique or Suite ID */
            plugInAETEComment,      /* optional description */
            {                       /* define inheritance */
             "<Inheritance>",       /* must be exactly this */
             keyInherits,           /* must be keyInherits */
             classFormat,           /* parent: Format, Import, Export */
             "parent class format", /* optional description */
             flagsSingleProperty,   /* if properties, list below */

             "Quality",
             keyWriteConfig_quality,
             typeInteger,
             "image quality after compression",
             flagsSingleProperty,

             "Compression",
             keyWriteConfig_compression,
             typeInteger,
             "compression level",
             flagsSingleProperty,

             "Using POSIX I/O",
             keyUsePOSIX,
             typeBoolean,
             "use POSIX file i/o",
             flagsSingleProperty},
            {}, /* elements (not supported) */
                /* class descriptions */
        },
        {}, /* comparison ops (not supported) */
        {}  /* any enumerations */
    }};

//------------------------------------------------------------------------------
// History resource
//------------------------------------------------------------------------------

resource StringResource(kHistoryEntry, "History",
                        purgeable){plugInName ": ref num=^0."};

//------------------------------------------------------------------------------

// end WebPShop.r
