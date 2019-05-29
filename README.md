# WebP file format plug-in for Photoshop

Current plug-in version: WebPShop 0.2.1

WebPShop is a Photoshop module for opening and saving WebP images, including
animations.

Please look at the files LICENSE and CONTRIBUTING in the "docs" folder before
using the contents of this repository or contributing.

## Installation

Current library version: WebP 1.0.2

Put the file from the `bin` folder in the Photoshop plug-in directory
(`C:\Program Files\Adobe\Adobe Photoshop\Plug-ins\WebPShop.8bi` for Windows x64,
`Applications/Adobe Photoshop/Plug-ins/WebPShop.plugin` for Mac). Run Photoshop.
That's it!

## Build

Use Microsoft Visual Studio (2017 and above) for Windows and XCode for Mac.

*   Download the latest Adobe Photoshop Plug-In and Connection SDK at
    https://console.adobe.io/downloads/ps,
*   Download the latest WebP binaries at
    https://developers.google.com/speed/webp/docs/precompiled or build them,
*   Put the contents of this repository in a "webpshop" folder located at
    `adobe_photoshop_sdk_[version]/pluginsdk/samplecode/format`,
*   Add `path/to/webp/includes` and `path/to/webp/includes/src` as Additional
    Include Directories to the WebPShop project [1],
*   Add webp, webpdemux, webpmux libraries as Additional Dependencies to the
    WebPShop project [1],
*   Build it with the same architecture as your Photoshop installation
    (x86 or x64 for Windows, x64 for Mac),
*   Build it with the same architecture and config as the WebP library binaries
    (x86 or x64 for Windows, x64 for Mac, Debug/Release),
*   By example for Windows, it should output the plug-in file `WebPShop.8bi` in
    `adobe_photoshop_sdk_[version]/pluginsdk/samplecode/Output/Win/Debug[64]/`.

[1] By default the XCode project includes and links to the
`libwebp-[version]-mac-[version]` folder in the `webpshop` directory. The VS
project expects `libwebp-[version]-windows-x64` (and/or `-x86`).

## Features

*   `Open`, `Open As` menu commands can be used to read .webp files.
*   `Save`, `Save As` menu commands can be used to write .webp files. Encoding
    parameters can be tuned through the UI.

![WebPShop encoding settings - Windows](docs/webpshop_enc_ui_windows.webp)

## Encoding settings

For information, the quality slider maps the following ranges to their internal
WebP counterparts (see SetWebPConfig() in WebPShopEncodeUtils.cpp):

    | Quality slider value   -> | 0    ...    97 | 98         99 |    100   |
    |---------------------------|----------------|---------------|----------|
    | WebP encoding settings -> | Lossy, quality | Near-lossless | Lossless |
    |                           | 0    ...   100 | 60         80 |          |

The radio buttons offer several levels of compression effort:

    | Label   | WebP speed setting  | Sharp YUV    | WebP "quality" setting |
    |         |                     | (lossy only) | (except for lossy)     |
    |---------|---------------------|--------------|------------------------|
    | Fastest |          1          |      No      |            0           |
    | Default |          4          |      No      |           75           |
    | Slowest |          6          |      Yes     |          100           |

## Limitations

*   Only English is currently supported.
*   Only "RGB Color" image mode is currently supported.
*   The Timeline data is not used; thus animations rely on layers for defining
    frames (set duration as "(123 ms)" in each layer's name), and they need to
    be rasterized before saving.
*   On some images, lossless compression might produce smaller file sizes than
    lossy. That's why the quality slider is not linear. The same problem exists
    with the radio buttons controlling the compression effort.
*   This plug-in does not extend `Export As` neither `Save for Web`.
*   Encoding and decoding are done in a single pass. It is not currently
    possible to cancel such actions, and it might take some time on big images.

## Software architecture

The `common` folder contains the following:

*   `WebPShop.h` is the main header, containing most functions.
*   `WebPShop.cpp` contains the plug-in entry point (called by host).
*   `WebPShop.r` and `WebPShopTerminology.h` represent the plug-in properties.
*   Functions in `WebPShopSelector*` are called in `WebPShop.cpp`.
*   `WebPShop*Utils.cpp` are helper functions.
*   `WebPShopScripting.cpp` is mostly used for automation.
*   `WebPShopUI*` display the encoding parameters window and the About box.

The `win` folder contains a Visual Studio solution and project, alongside with
`WebPShop.rc` which is the encoding parameters window layout and About box.

The `mac` folder contains an XCode project. `WebPShopUIDialog_mac.h` and `.mm`
describe the UI layout, while `WebPShopUI_mac.mm` handles the window events.
