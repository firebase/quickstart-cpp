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

- [Read more about Firebase Auth](https://firebase.google.com/docs/auth/)

Building and Running the testapp
--------------------------------

### iOS
  - Link your iOS app to the Firebase libraries.
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
      [general instructions for setting up an iOS app with Firebase](https://firebase.google.com/docs/ios/setup).
  - Register your iOS app with Firebase.
    - Create a new app on the [Firebase console](https://firebase.google.com/console/), and attach
      your iOS app to it.
      - You can use "com.google.ios.auth.testapp" as the iOS Bundle ID
        while you're testing. You can omit App Store ID while testing.
    - Add the GoogleService-Info.plist that you downloaded from Firebase
      console to the testapp root directory. This file identifies your iOS app
      to the Firebase backend.
    - In Firebase console, select "Auth", then enable "Email/Password", and also
      enable "Anonymous". This will allow the testapp to use email accounts and
      anonymous sign-in.
  - Download the Firebase C++ SDK linked from
    [https://firebase.google.com/docs/cpp/setup]() and unzip it to a
    directory of your choice.
  - Add the following frameworks from the Firebase C++ SDK to the project:
    - frameworks/ios/universal/firebase.framework
    - frameworks/ios/universal/firebase_auth.framework
    - You will need to either,
       1. Check "Copy items if needed" when adding the frameworks, or
       2. Add the framework path in "Framework Search Paths"
          - e.g. If you downloaded the Firebase C++ SDK to
            `/Users/me/firebase_cpp_sdk`,
            then you would add the path
            `/Users/me/firebase_cpp_sdk/frameworks/ios/universal`.
          - To add the path, in XCode, select your project in the project
            navigator, then select your target in the main window.
            Select the "Build Settings" tab, and click "All" to see all
            the build settings. Scroll down to "Search Paths", and add
            your path to "Framework Search Paths".
  - In XCode, build & run the sample on an iOS device or simulator.
  - The testapp has no user interface. The output of the app can be viewed
    via the console.  In Xcode,  select
    "View --> Debug Area --> Activate Console" from the menu.
  - See [below](#using_the_test_app) for usage instructions.

### Android
  - Register your Android app with Firebase.
    - Create a new app on the [Firebase console](https://firebase.google.com/console/), and attach
      your Android app to it.
      - You can use "com.google.android.auth.testapp" as the Package Name
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
    - In Firebase console, select "Auth", then enable "Email/Password", and also
      enable "Anonymous". This will allow the testapp to use email accounts and
      anonymous sign-in.
    - For further details please refer to the
      [general instructions for setting up an Android app with Firebase](https://firebase.google.com/docs/android/setup).
  - Download the Firebase C++ SDK linked from
    [https://firebase.google.com/docs/cpp/setup]() and unzip it to a
    directory of your choice.
  - Configure the location of the Firebase C++ SDK by setting the
    firebase\_cpp\_sdk.dir Gradle property to the SDK install directory.
    For example, in the project directory:
      ```
      > echo "systemProp.firebase\_cpp\_sdk.dir=/User/$USER/firebase\_cpp\_sdk" >> gradle.properties
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
  - Build the testapp and run it on an Android device or emulator.
  - The testapp has no user interface. The output of the app can be viewed
    in the logcat output of Android studio or by running
    "adb logcat *:W android_main firebase" from the command line.

Known issues
------------

  - Auth::SignInAnonymously() and Auth::CreateUserWithEmailAndPassword() are
    temporarily broken. For the moment, they will always return
    kAuthError_GeneralBackendError.
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
  - When given invalid parameters, several API functions return
    kAuthError_GeneralBackendError instead of kAuthError_InvalidEmail or
    kAuthError_EmailNotFound.

Support
-------

[https://firebase.google.com/support/]()

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
