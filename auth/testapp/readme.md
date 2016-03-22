Firebase Auth Quickstart
========================

The Firebase Auth Test Application (testapp) demonstrates authentication
and user profile operations with the Firebase Auth C++ SDK. The application has
no user interface and simply logs actions it's performing to the console.

The testapp performs the following:
  - Creates a firebase::App in a platform-specific way. The App holds
    platform-specific context that's used by other Firebase APIs.
  - Gets a pointer to firebase::Auth. The Auth class is the gateway to all
    Firebase Authentication functionality.
  - Calls every member function on firebase::Auth. Many of these functions are
    asynchronous, since they communicate with a server. Asynchronous functions
    return a firebase::Future class, which we wait on until the call completes.
    In practice, you will probably want to register a callback on the Future,
    or poll it periodically instead of waiting for it to complete.
  - Gets a pointer to firebase::User. The User class allows account manipulation
    and linking. It's returned by every Auth sign-in operation, and the
    currently active User is available via Auth::CurrentUser(). Only one User
    can be active at a time.
  - Calls every member function on firebase::User.

Introduction
------------

- [Read more about Firebase Auth](https://developers.google.com/firebase/)

Getting Started
---------------

### iOS
  - [Add Firebase to your iOS Project](https://developers.google.com/firebase/docs/ios/setup).
    You can use "com.google.ios.auth.testapp" as the App ID while
    you're testing.
  - Add the following frameworks from the Firebase C++ SDK to the project:
    - frameworks/ios/universal/firebase.framework
    - frameworks/ios/universal/firebase_auth.framework
  - From XCode build & run the sample on an iOS device or emulator.
  - The application has no user interface, the output of the app can be viewed
    via the console.  In Xcode,  select
    "View --> Debug Area --> Activate Console" from the menu.

### Android
  - [Add Firebase to your Android Project](https://developers.google.com/firebase/docs/android/setup).
    You can use "com.google.android.auth.testapp" as the App ID while
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
  - The application has no user interface, the output of the app can be viewed
    in the logcat output of Android studio or by running
    "adb logcat *:W android_main firebase" from the command line.

Known issues
------------
  - User::UpdateUserProfile() currently fails to set the photo URL on both
    Android and iOS, and also fails to set the display name on Android.
  - The iOS testapp generates benign asserts of the form:
         ```
         assertion failed: 15D21 13C75: assertiond + 12188
         [8CF1968D-3466-38B7-B225-3F6F5B64C552]: 0x1
         ```
  - Several API function are pending competion and will return status
    `firebase::kAuthError_Unimplemented` or empty data.
      - `Auth::ConfirmPasswordReset()`
      - `Auth::CheckActionCode()`
      - `Auth::ApplyActionCode()`
      - `User::Reauthenticate()`
      - `User::SendEmailVerification()`
      - `User::Delete()`
      - `User::ProviderData()`
  - Several API functions return different error codes on iOS and Android.
    The disparities will be eliminated in a subsequent release.

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

