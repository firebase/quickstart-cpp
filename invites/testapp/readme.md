Firebase Invites Quickstart
==============================

The Firebase Invites Test Application (testapp) demonstrates both
sending and receiving Firebase Invites using the Firebase Invites C++
SDK. This application has no user interface and simply logs actions
it's performing to the console.

Introduction
------------

- [Read more about Firebase Invites](https://developers.google.com/app-invites/)
- [Read more about Firebase Durable Links](https://developers.google.com/firebase/docs/durable-links/)

Getting Started
---------------

### iOS
  - [Add Firebase to your iOS Project](https://developers.google.com/firebase/docs/ios/setup).
    - Follow the
      [quickstart guide](https://developers.google.com/firebase/docs/ios/setup)
      to set up your project.
    - Follow the "Get a configuration file" and "Add the configuration file to
    your project" steps from the
    [Try App Invites on iOS](https://developers.google.com/app-invites/ios/guides/start)
    doc to configure the project.
    -  Use "com.google.ios.invites.testapp" as the App ID while you're testing,
    and a random App Store ID such as 12345678.
  - Add the following frameworks from the Firebase C++ SDK to the project:
    - frameworks/ios/universal/firebase.framework
    - frameworks/ios/universal/firebase_invites.framework
  - From XCode build & run the sample on an iOS device or emulator.
  - The application has no user interface, the output of the app can be viewed
    via the console.  In Xcode,  select
    "View --> Debug Area --> Activate Console" from the menu.
  - See [below](#using_the_test_app) for usage instructions.

### Android
  - [Add Firebase to your Android Project](https://developers.google.com/firebase/docs/android/setup).
    - Follow the
    [quickstart guide](https://developers.google.com/firebase/docs/android/setup)
    to set up your project.
    - Follow the "Get a configuration file" and "Add the configuration file to
    your project" steps from the
    [Try App Invites on Android](https://developers.google.com/app-invites/android/guides/start)
    doc to configure the project.
    - Use "com.google.android.invites.testapp" as the App ID while are testing.
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
  - See [below](#using_the_test_app) for usage instructions.

# Using the Test App

- Install and run the test app on your iOS or Android device or emulator.
- The application has minimal user interface. The output of the app can be viewed
  via the console:
  - __iOS__: Open select "View --> Debug Area --> Activate Console" from the menu
    in Xcode.
  - __Android__: View the logcat output in Android studio or by running
    "adb logcat" from the command line.

- When you first run the app, it will check for an incoming durable link or
  invitation, and report whether it was able to fetch an invite.
- Afterwards, it will open a screen that allows you to send an invite for the
  current app via e-mail or SMS.
  - You may have to log in to Google first.
- To simulate receiving an invitation from a friend, you can send yourself an
  invite, uninstall the test app, then click the link in your e-mail.
  - This would normally send you to the Play Store or App Store to download the
    app. Because this is a test app, it will link to a nonexistent store page.
- After clicking the invite link, re-install and run the app on your device or
  emulator, and see the invitation fetched on the receiving side.

Support
-------

https://developers.google.com/firebase/support/

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
