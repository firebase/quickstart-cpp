// Copyright 2016 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef FIREBASE_TESTAPP_MAIN_H_  // NOLINT
#define FIREBASE_TESTAPP_MAIN_H_  // NOLINT

#if defined(__ANDROID__)
#include <android/native_activity.h>
#include <jni.h>
#endif  // defined(__ANDROID__)

// Defined using -DANDROID_MAIN_APP_NAME=some_app_name when compiling this
// file.
#ifndef FIREBASE_TESTAPP_NAME
#define FIREBASE_TESTAPP_NAME "android_main"
#endif  // FIREBASE_TESTAPP_NAME

// Cross platform logging method.
// Implemented by android/android_main.cc or ios/ios_main.cc.
extern "C" int LogMessage(const char* format, ...);

#if defined(__ANDROID__)
// Android specific method to flush pending events for the main thread.
bool ProcessAndroidEvents(int msec);
// Get the JNI environment.
JNIEnv* GetJniEnv();
// Get the activity.
jobject GetActivity();
#endif  // defined(__ANDROID__)

#endif  // FIREBASE_TESTAPP_MAIN_H_  // NOLINT
