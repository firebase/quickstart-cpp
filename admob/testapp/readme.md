Firebase AdMob Quickstart
==============================

The Firebase AdMob Test Application (testapp) demonstrates loading and showing
banners and interstitials using the Firebase AdMob C++ SDK. The application
has no user interface and simply logs actions it's performing to the console
while displaying the ads.

Introduction
------------

- [Read more about Firebase AdMob](https://firebase.google.com/docs/admob)

Getting Started
---------------

### iOS
 - Link your iOS app to the Firebase libraries.
    - Get CocoaPods version 1 or later by running,
        ```
        sudo gem install cocoapods --pre
        ```
    - From the testapp directory, install the CocoaPods listed in the Podfile
      by running,
        ```
        pod install
        ```
    - Open the generated Xcode workspace (which now has the CocoaPods),
        ```
        open testapp.xcworkspace
        ```
    - For further details please refer to the
      [general instructions for setting up an iOS app with Firebase](https://firebase.google.com/docs/ios/setup).
  - Register your iOS app with Firebase.
    - Create a new app on the [Firebase console](https://firebase.google.com/console/),
      and attach your iOS app to it.
      - You can use "com.google.ios.admob.testapp" as the iOS Bundle ID
        while you're testing. You can omit App Store ID while testing.
  - Download the Firebase C++ SDK linked from
    [https://firebase.google.com/docs/cpp/setup](https://firebase.google.com/docs/cpp/setup)
    and unzip it to a directory of your choice.
  - Add the following frameworks from the Firebase C++ SDK to the project:
    - frameworks/ios/universal/firebase.framework
    - frameworks/ios/universal/firebase_admob.framework
    - You will need to either,
       1. Check "Copy items if needed" when adding the frameworks, or
       2. Add the framework path in "Framework Search Paths"
          - For example, if you downloaded the Firebase C++ SDK to
            `/Users/me/firebase_cpp_sdk`,
            then you would add the path
            `/Users/me/firebase_cpp_sdk/frameworks/ios/universal`.
          - To add the path, in Xcode, select your project in the project
            navigator, then select your target in the main window.
            Select the "Build Settings" tab, and click "All" to see all
            the build settings. Scroll down to "Search Paths", and add
            your path to "Framework Search Paths".
  - Update the AdMob App ID:
    - In the `src/common_main.cc`, update `kAdMobAppID` with the app ID for
      your iOS app, replacing 'YOUR_IOS_ADMOB_APP_ID'.
    - In the `testapp/Info.plist`, update `GADApplicationIdentifier` with the
      same app ID, replacing 'YOUR_IOS_ADMOB_APP_ID'.
    - For more information, see
      [Update your Info.plist](https://developers.google.com/admob/ios/quick-start#manual_download)
  - In Xcode, build & run the sample on an iOS device or simulator.
  - The testapp displays a banner ad and an interstitial ad. You can dismiss
    the interstitial ad to see the banner ad.
  - Afterwards, the testapp will display a Rewarded Video test ad.
  - The output of the app can be viewed onscreen or via the console. To view
    the console in Xcode, select "View --> Debug Area --> Activate Console"
    from the menu.

### Android
  - Register your Android app with Firebase.
    - Create a new app on the [Firebase console](https://firebase.google.com/console/),
      and attach your Android app to it.
      - You can use "com.google.android.admob.testapp" as the Package Name
        while you're testing.
      - To [generate a SHA1](https://developers.google.com/android/guides/client-auth)
        run this command on Mac and Linux,
        ```
        keytool -exportcert -list -v -alias androiddebugkey -keystore ~/.android/debug.keystore
        ```
        or this command on Windows,
        ```
        keytool -exportcert -list -v -alias androiddebugkey -keystore %USERPROFILE%\.android\debug.keystore
        ```
      - If keytool reports that you do not have a debug.keystore, you can
        [create one with](http://developer.android.com/tools/publishing/app-signing.html#signing-manually),
        ```
        keytool -genkey -v -keystore ~/.android/debug.keystore -storepass android -alias androiddebugkey -keypass android -dname "CN=Android Debug,O=Android,C=US"
        ```
    - Add the `google-services.json` file that you downloaded from Firebase
      console to the root directory of testapp. This file identifies your
      Android app to the Firebase backend.
    - For further details please refer to the
      [general instructions for setting up an Android app with Firebase](https://firebase.google.com/docs/android/setup).
  - Download the Firebase C++ SDK linked from
    [https://firebase.google.com/docs/cpp/setup](https://firebase.google.com/docs/cpp/setup)
    and unzip it to a directory of your choice.
  - Configure the location of the Firebase C++ SDK by setting the
    firebase\_cpp\_sdk.dir Gradle property to the SDK install directory.
    For example, in the project directory:
      ```
      echo "systemProp.firebase\_cpp\_sdk.dir=/User/$USER/firebase\_cpp\_sdk" >> gradle.properties
      ```
  - Ensure the Android SDK and NDK locations are set in Android Studio.
    - From the Android Studio launch menu, go to `File/Project Structure...` or
      `Configure/Project Defaults/Project Structure...`
      (Shortcut: Control + Alt + Shift + S on windows,  Command + ";" on a mac)
      and download the SDK and NDK if the locations are not yet set.
  - Open *build.gradle* in Android Studio.
    - From the Android Studio launch menu, "Open an existing Android Studio
      project", and select `build.gradle`.
  - Install the SDK Platforms that Android Studio reports missing.
  - Update the AdMob App ID:
    - In the `src/common_main.cc`, update `kAdMobAppID` with the app ID for
      your Android app, replacing 'YOUR_ANDROID_ADMOB_APP_ID'.
    - In the `AndroidManifest.xml`, update
      `com.google.android.gms.ads.APPLICATION_ID` with the same app ID,
      replacing 'YOUR_ANDROID_ADMOB_APP_ID'.
    - For more information, see
      [Update your AndroidManifest.xml](https://developers.google.com/admob/android/quick-start#update_your_androidmanifestxml)
  - Build the testapp and run it on an Android device or emulator.
  - The testapp will initialize AdMob, then load and display a test banner and
    a test interstitial.
  - Tapping on an ad to verify the clickthrough process is possible, and the
    interstitial will wait to be closed by the user.
  - Afterwards, the testapp will display a Rewarded Video test ad.
  - While this is happening, information from the device log will be written
    to an onscreen TextView.
    - Logcat can also be used as normal.

### Desktop
  - Register your app with Firebase.
    - Create a new app on the [Firebase console](https://firebase.google.com/console/),
      following the above instructions for Android or iOS.
    - If you have an Android project, add the `google-services.json` file that
      you downloaded from the Firebase console to the root directory of the
      testapp.
    - If you have an iOS project, and don't wish to use an Android project,
      you can use the Python script `generate_xml_from_google_services_json.py --plist`,
      located in the Firebase C++ SDK, to convert your `GoogleService-Info.plist`
      file into a `google-services-desktop.json` file, which can then be
      placed in the root directory of the testapp.
  - Download the Firebase C++ SDK linked from
    [https://firebase.google.com/docs/cpp/setup](https://firebase.google.com/docs/cpp/setup)
    and unzip it to a directory of your choice.
  - Configure the testapp with the location of the Firebase C++ SDK.
    This can be done a couple different ways:
    - When invoking cmake, pass in the location with
      -DFIREBASE_CPP_SDK_DIR=/path/to/firebase_cpp_sdk.
    - Set an environment variable for FIREBASE_CPP_SDK_DIR to the path to use.
    - Edit the CMakeLists.txt file, changing the FIREBASE_CPP_SDK_DIR path
      to the appropriate location.
  - From the testapp directory, generate the build files by running,
      ```
      cmake .
      ```
    If you want to use XCode, you can use -G"Xcode" to generate the project.
    Similarly, to use Visual Studio, -G"Visual Studio 15 2017". For more
    information, see
    [CMake generators](https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html).
  - Build the testapp, by either opening the generated project file based on
    the platform, or running,
      ```
      cmake --build .
      ```
  - Execute the testapp by running,
      ```
      ./desktop_testapp
      ```
    Note that the executable might be under another directory, such as Debug.
  - The testapp has no user interface, but the output can be viewed via the
    console. Note that Admob uses a stubbed implementation on desktop,
    so functionality is not expected.

Support
-------

[https://firebase.google.com/support/](https://firebase.google.com/support/)

License
-------

Copyright 2016 Google, Inc.

Licensed to the Apache Software Foundation (ASF) under one or more contributor
license agreements.  See the NOTICE file distributed with this work for
additional information regarding copyright ownership.  The ASF licenses this
file to you under the Apache License, Version 2.0 (the "License"); you may not
use this file except in compliance with the License.  You may obtain a copy of
the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
License for the specific language governing permissions and limitations under
the License.
