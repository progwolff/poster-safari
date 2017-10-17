# Build

## Requirenments

Software:
* nodejs
* npm
* Optional:
  * android-sdk / android-studio

## Install

1. Install Ionic: `npm install -g cordova ionic`
1. `npm update`

## Icon and Splash

To generate a new icon or spash image, create files `resources/icon.png` or `resources/splash.png` and generate artifacts: `ionic cordova resources`

An ionic account is neccessary.

## Documentation

To create code documentation from source files, run `ǹpm run doc`. The docs will be created in `docs/index.html`.

# Run

## Debug app in web browser

Doesn't work because of network-plugin...

However, would be done as follows:

Start server:  `ionic serve `

Visit: `http://localhost:8100/ionic-lab`

## Android emulator

### Install

#### Aur-Packages

* android-platform
* android-sdk
* android-sdk-build-tools
* android-sdk-platform-tools

Add android platform to project: `ionic platform add android`

### Start app in emulator

To start the app in android emulator:

1. Start AVD Mangager `<android-sdk-location>/tools/android avd` and create a new device
1. Start emulator: `ionic run android`

### Start app on device

1. Connect your smartphone via USB
1. Enable USB debugging on your smartphone
1. check if adb detects device: `adb devices` should show `<id>	device`. On error it shows: `<id> unauthorized`
1. Run `ionic cordova run android` auführen.
    * Debug logs: `adb logcat -s chromium` or `adb logcat | grep -i -e chrom -e poster -e pouch`
