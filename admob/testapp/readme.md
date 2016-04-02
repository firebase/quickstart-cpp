Firebase AdMob Quickstart
==============================

The Firebase AdMob Test Application (testapp) demonstrates loading and showing
banners and interstitials using the Firebase AdMob C++ SDK. The application
has no user interface and simply logs actions it's performing to the console
while displaying the ads.

Introduction
------------

- [Read more about Firebase AdMob](https://developers.google.com/firebase/)

Getting Started
---------------

### iOS
 - Link your iOS app to the Firebase libraries.
    - Get access to the Firebase SDK git repo via
      [git cookie](https://cpdc-eap.googlesource.com/new-password).
    - Get CocoaPods version 1 or later by running,
        ```
        $ sudo gem install CocoaPods --pre
        ```
    - From the testapp directory, install the CocoaPods listed in the Podfile
      by running,
        ```
        $ pod install
        ```
    - Open the generated Xcode workspace (which now has the CocoaPods),
        ```
        $ open testapp.xcworkspace
        ```
    - For further details please refer to the
      [general instructions for setting up an iOS app with Firebase](https://developers.google.com/firebase/docs/ios/setup).
  - Register your iOS app with Firebase.
    - Create a new app on [Firebase console](https://g.co/firebase), and attach
      your iOS app to it.
      - You can use "com.google.ios.admob.testapp" as the iOS Bundle ID
        while you're testing. You can omit App Store ID while testing.
  - Add the following frameworks from the Firebase C++ SDK to the project:
    - frameworks/ios/universal/firebase.framework
    - frameworks/ios/universal/firebase_admob.framework
    - You will need to either,
       1. Check "Copy items if needed" when adding the frameworks, or
       2. Add the framework path in "Framework Search Paths"
          - e.g. If you downloaded the Firebase C++ SDK to
            `/Users/me/firebase_cpp_sdk`,
            then you would add the path
            `/Users/me/firebase_cpp_sdk/frameworks/ios/universal`.
          - To add the path, in Xcode, select your project in the project
            navigator, then select your target in the main window.
            Select the "Build Settings" tab, and click "All" to see all
            the build settings. Scroll down to "Search Paths", and add
            your path to "Framework Search Paths".
  - In Xcode, build & run the sample on an iOS device or simulator.
  - The testapp displays a banner ad and an interstitial ad. You can
    dismiss the interstitial ad to see the banner ad. The output of the app can
    be viewed via the console. To view the conscole in Xcode, select
    "View --> Debug Area --> Activate Console" from the menu.

### Android
  - [Add Firebase to your Android Project](https://developers.google.com/firebase/docs/android/setup).
    You can use "com.google.android.admob.testapp" as the App ID while
    you're testing.
  - Configure the location of the Firebase C++ SDK by setting the
    firebase\_cpp\_sdk.dir Gradle property to the SDK install directory.
    For example, in the project directory:
    > echo "systemProp.firebase\_cpp\_sdk.dir=~/firebase\_cpp\_sdk" >> gradle.properties
  - Configure the location of the Android NDK by setting the ndk.dir Gradle
    property to the NDK install directory.
    For example, in the project directory:
    > echo "ndk.dir=~/ndk" >> local.properties
  - Open *build.gradle* in Android Studio.
  - Build & run the sample on an Android device or emulator.
  - The application displays ads, but has no other user interaction. The output
    of the app can be viewed in the logcat output of Android studio or by
    running "adb logcat" from the command line.

Support
-------

[https://developers.google.com/firebase/support/]()

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
