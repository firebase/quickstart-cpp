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

#include <inttypes.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <sstream>
#include "firebase/app.h"
#include "firebase/auth.h"
#include "firebase/functions.h"
#include "firebase/future.h"
#include "firebase/log.h"
#include "firebase/util.h"

// Thin OS abstraction layer.
#include "main.h"  // NOLINT

// Wait for a Future to be completed. If the Future returns an error, it will
// be logged.
void WaitForCompletion(const firebase::FutureBase& future, const char* name) {
  while (future.status() == firebase::kFutureStatusPending) {
    ProcessEvents(100);
  }
}


extern "C" int common_main(int argc, const char* argv[]) {
  ::firebase::App* app;

#if defined(__ANDROID__)
  app = ::firebase::App::Create(GetJniEnv(), GetActivity());
#else
  app = ::firebase::App::Create();
#endif  // defined(__ANDROID__)

  LogMessage("Initialized Firebase App.");

  LogMessage("Initializing Firebase Auth and Cloud Functions.");

  // Use ModuleInitializer to initialize both Auth and Functions, ensuring no
  // dependencies are missing.
  ::firebase::functions::Functions* functions = nullptr;
  ::firebase::auth::Auth* auth = nullptr;
  void* initialize_targets[] = {&auth, &functions};

  const firebase::ModuleInitializer::InitializerFn initializers[] = {
      [](::firebase::App* app, void* data) {
        LogMessage("Attempt to initialize Firebase Auth.");
        void** targets = reinterpret_cast<void**>(data);
        ::firebase::InitResult result;
        *reinterpret_cast<::firebase::auth::Auth**>(targets[0]) =
            ::firebase::auth::Auth::GetAuth(app, &result);
        return result;
      },
      [](::firebase::App* app, void* data) {
        LogMessage("Attempt to initialize Cloud Functions.");
        void** targets = reinterpret_cast<void**>(data);
        ::firebase::InitResult result;
        *reinterpret_cast<::firebase::functions::Functions**>(targets[1]) =
            ::firebase::functions::Functions::GetInstance(app, &result);
        return result;
      }};

  ::firebase::ModuleInitializer initializer;
  initializer.Initialize(app, initialize_targets, initializers,
                         sizeof(initializers) / sizeof(initializers[0]));

  WaitForCompletion(initializer.InitializeLastResult(), "Initialize");

  if (initializer.InitializeLastResult().error() != 0) {
    LogMessage("Failed to initialize Firebase libraries: %s",
               initializer.InitializeLastResult().error_message());
    ProcessEvents(2000);
    return 1;
  }
  LogMessage("Successfully initialized Firebase Auth and Cloud Functions.");

  // To test against a local emulator, uncomment this line:
  //   functions->UseFunctionsEmulator("http://localhost:5005");
  // Or when running in an Android emulator:
  //   functions->UseFunctionsEmulator("http://10.0.2.2:5005");

  // Optionally, sign in using Auth before accessing Functions.
  {
    firebase::Future<firebase::auth::User*> sign_in_future =
        auth->SignInAnonymously();
    WaitForCompletion(sign_in_future, "SignInAnonymously");
    if (sign_in_future.error() == firebase::auth::kAuthErrorNone) {
      LogMessage("Auth: Signed in anonymously.");
    } else {
      LogMessage("ERROR: Could not sign in anonymously. Error %d: %s",
                 sign_in_future.error(), sign_in_future.error_message());
      LogMessage(
          "  Ensure your application has the Anonymous sign-in provider "
          "enabled in Firebase Console.");
      LogMessage(
          "  Attempting to connect to Cloud Functions anyway. This may fail "
          "depending on the function.");
    }
  }


  // Create a callable.
  LogMessage("Calling addNumbers");
  firebase::functions::HttpsCallableReference addNumbers;
  addNumbers = functions->GetHttpsCallable("addNumbers");

  firebase::Future<firebase::functions::HttpsCallableResult> future;
  {
    std::map<std::string, firebase::Variant> data;
    data["firstNumber"] = firebase::Variant(5);
    data["secondNumber"] = firebase::Variant(7);
    future = addNumbers.Call(firebase::Variant(data));
  }
  WaitForCompletion(future, "Call");
  if (future.error() != firebase::functions::kErrorNone) {
    LogMessage("FAILED!");
    LogMessage("  Error %d: %s", future.error(), future.error_message());
  } else {
    firebase::Variant result = future.result()->data();
    int op_result =
        static_cast<int>(result.map()["operationResult"].int64_value());
    const int expected = 12;
    if (op_result != expected) {
      LogMessage("FAILED!");
      LogMessage("  Expected: %d, Actual: %d", expected, op_result);
    } else {
      LogMessage("SUCCESS.");
      LogMessage("  Got expected result: %d", op_result);
    }
  }

  LogMessage("Shutting down the Functions library.");
  delete functions;
  functions = nullptr;

  // Ensure that the ref we had is now invalid.
  if (!addNumbers.is_valid()) {
    LogMessage("SUCCESS: Reference was invalidated on library shutdown.");
  } else {
    LogMessage("ERROR: Reference is still valid after library shutdown.");
  }

  LogMessage("Signing out from anonymous account.");
  auth->SignOut();
  LogMessage("Shutting down the Auth library.");
  delete auth;
  auth = nullptr;

  LogMessage("Shutting down Firebase App.");
  delete app;

  // Wait until the user wants to quit the app.
  while (!ProcessEvents(1000)) {
  }

  return 0;
}
