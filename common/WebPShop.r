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

#define plugInName "WebPShop"
#define plugInCopyrightYear "2020"
#define plugInDescription \
  "A Photoshop plug-in for reading and writing WebP files."

//------------------------------------------------------------------------------
// Definitions -- Required by other resources in this resources file.
//------------------------------------------------------------------------------

// Dictionary (aete) resources:

#define webpshopResourceID 16000  // number seems standard
#define vendorName "Google"
#define plugInAETEComment "webp format module"

#define plugInSuiteID 'gOOG'
#define plugInClassID 'webP'
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

resource 'PiPL'(webpshopResourceID, plugInName " PiPL", purgeable){
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
     HasTerminology{plugInClassID, plugInEventID, webpshopResourceID,
                    vendorName " " plugInName},

     SupportedModes{noBitmap, noGrayScale, noIndexedColor, doesSupportRGBColor,
                    noCMYKColor, noHSLColor, noHSBColor, noMultichannel,
                    noDuotone, noLABColor},
     EnableInfo{"in (PSHOP_ImageMode, RGBMode, RGBColorMode)"},

     // WebP is limited to 16383 in width and height.
     PlugInMaxSize{16383, 16383}, FormatMaxSize{{16383, 16383}},

     FormatMaxChannels{{1, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24}},

     FmtFileType{'WEBP', '8BIM'},
     ReadTypes{{'WEBP', '    '}},
     // FilteredTypes{{'8B1F', '    '}},
     ReadExtensions{{'WEBP'}},
     WriteExtensions{{'WEBP'}},
     FilteredExtensions{{'WEBP'}},
     FormatFlags{fmtSavesImageResources, fmtCanRead, fmtCanWrite,
                 fmtCanWriteIfRead, fmtCanWriteTransparency,
                 fmtCannotCreateThumbnail},
     FormatICCFlags{iccCannotEmbedGray, iccCannotEmbedIndexed,
                    iccCanEmbedRGB, iccCannotEmbedCMYK},

     // XMPRead and XMPWrite properties would enable formatSelectorXMPRead
	   // and formatSelectorXMPWrite which represent another way of handling
	   // XMP metadata import/export. Currently it is done as EXIF: through
	   // handle allocation and setPropertyProc().

	   // No way of enabling the "ICC Profile" checkbox in the "Save As" window
	   // was found. Currently it is configurable in the "WebPShop" window among
	   // other WebP settings and internally handled in a similar way as EXIF and
	   // XMP chunks, using the iCCprofileData handle.

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

resource 'aete'(webpshopResourceID, plugInName " dictionary", purgeable){
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

             "Keep EXIF",
             keyWriteConfig_keep_exif,
             typeBoolean,
             "keep EXIF metadata",
             flagsSingleProperty,

             "Keep XMP",
             keyWriteConfig_keep_xmp,
             typeBoolean,
             "keep XMP metadata",
             flagsSingleProperty,

             "Keep Color Profile",
             keyWriteConfig_keep_color_profile,
             typeBoolean,
             "keep color profile",
             flagsSingleProperty,

             "Infinite Loop",
             keyWriteConfig_loop_forever,
             typeBoolean,
             "loop the animation forever",
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

resource 'STR '(kHistoryEntry, "History", purgeable){
  plugInName ": ref num=^0."
};

//------------------------------------------------------------------------------

// end WebPShop.r
