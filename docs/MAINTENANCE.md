# Maintenance

Here are described some tips on maintaining the Photoshop plug-in "WebPShop".

## Updating WebP version

To update the WebP **library** version linked in the WebPShop binaries, do the
following:

*   Update the About box text in `win/WebPShop.rc` and
    `mac/WebPShopUI_mac.mm` to the new WebP library version.

*   Update the WebP library version in `README.md`.

*   Build WebPShop (Release) for each supported OS/plaform pair and upload.

## Updating WebPShop version

To update the WebPShop **plug-in** version, do the following:

*   Update the About box text in `win/WebPShop.rc` and
    `mac/WebPShopUI_mac.mm` to the new WebPShop version and to the current
    copyright year (also in `common/WebPShop.r`).

*   Update the WebPShop version in `README.md`, `mac/Info.plist` and
    `win/project.pbxproj`.

*   Describe the changes in `docs/NEWS`.

*   Build WebPShop (Release) for each supported OS/plaform pair and upload.
