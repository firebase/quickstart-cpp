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

#include "firebase/app.h"
#include "firebase/auth.h"

// Thin OS abstraction layer.
#include "main.h"  // NOLINT

::firebase::App* g_app;

// Execute all methods of the C++ Auth API.
extern "C" int common_main(int argc, const char* argv[]) {
  LogMessage("Initialize the Auth library");
#if defined(__ANDROID__)
  g_app = ::firebase::App::Create(::firebase::AppOptions(FIREBASE_TESTAPP_NAME),
                                  GetJniEnv(), GetActivity());
#else
  g_app =
      ::firebase::App::Create(::firebase::AppOptions(FIREBASE_TESTAPP_NAME));
#endif  // defined(__ANDROID__)

  LogMessage("Created the Firebase app %x",
             static_cast<int>(reinterpret_cast<intptr_t>(g_app)));

  // TODO(jsanmiya): Get more recent Auth jars.
  // The ones in //third_party/java/android_libs/gcore:gcore_urda_auth_1p are
  // too stale.
  // firebase::Auth* auth = firebase::Auth::GetAuth(g_app);
  // LogMessage("Created the Auth %x class for the Firebase app",
  //            static_cast<int>(reinterpret_cast<intptr_t>(auth)));

  return 0;
}
