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

#include <string>

#if defined(__ANDROID__)
#include <android/native_activity.h>
#include <jni.h>
#elif defined(__APPLE__)
#include <TargetConditionals.h>
extern "C" {
#include <objc/objc.h>
}  // extern "C"
#endif  // __ANDROID__, __APPLE__

// Defined using -DANDROID_MAIN_APP_NAME=some_app_name when compiling this
// file.
#ifndef FIREBASE_TESTAPP_NAME
#define FIREBASE_TESTAPP_NAME "android_main"
#endif  // FIREBASE_TESTAPP_NAME

// Cross platform logging method.
// Implemented by android/android_main.cc or ios/ios_main.mm.
extern "C" void LogMessage(const char* format, ...);

// Platform-independent method to flush pending events for the main thread.
// Returns true when an event requesting program-exit is received.
bool ProcessEvents(int msec);

// WindowContext represents the handle to the parent window.  It's type
// (and usage) vary based on the OS.
#if defined(__ANDROID__)
typedef jobject WindowContext;  // A jobject to the Java Activity.
#elif TARGET_OS_IPHONE
typedef id WindowContext;  // A pointer to an iOS UIView.
#else
typedef void* WindowContext;  // A void* for any other environments.
#endif

#if defined(__ANDROID__)
// Get the JNI environment.
JNIEnv* GetJniEnv();
// Get the activity.
jobject GetActivity();
#endif  // defined(__ANDROID__)

// Returns a variable that describes the window context for the app. On Android
// this will be a jobject pointing to the Activity. On iOS, it's an id pointing
// to the root view of the view controller.
WindowContext GetWindowContext();

// Prompt the user with a dialog box to enter a line of text, blocking
// until the user enters the text or the dialog box is canceled.
// Returns the text that was entered, or an empty string if the user
// canceled.
std::string ReadTextInput(const char* title, const char* message,
                          const char* placeholder);

#endif  // FIREBASE_TESTAPP_MAIN_H_  // NOLINT
