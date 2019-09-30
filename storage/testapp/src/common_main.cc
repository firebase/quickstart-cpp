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

#include "firebase/app.h"
#include "firebase/auth.h"
#include "firebase/future.h"
#include "firebase/log.h"
#include "firebase/storage.h"
#include "firebase/util.h"

// Thin OS abstraction layer.
#include "main.h"  // NOLINT

using app_framework::GetCurrentTimeInMicroseconds;
using app_framework::LogMessage;
using app_framework::ProcessEvents;

const char* kPutFileTestFile = "PutFileTest.txt";
const char* kGetFileTestFile = "GetFileTest.txt";

// Optionally set this to your Cloud Storage URL (gs://...) to test
// in a specific Cloud Storage bucket.
const char* kStorageUrl = nullptr;

// Wait for a Future to be completed. If the Future returns an error, it will
// be logged.
void WaitForCompletion(const firebase::FutureBase& future, const char* name) {
  while (future.status() == firebase::kFutureStatusPending) {
    ProcessEvents(100);
  }
  if (future.status() != firebase::kFutureStatusComplete) {
    LogMessage("ERROR: %s returned an invalid result.", name);
  } else if (future.error() != 0) {
    LogMessage("ERROR: %s returned error %d: %s", name, future.error(),
               future.error_message());
  }
}

extern "C" int common_main(int argc, const char* argv[]) {
  ::firebase::App* app;

#if defined(__ANDROID__)
  app = ::firebase::App::Create(app_framework::GetJniEnv(),
                                app_framework::GetActivity());
#else
  app = ::firebase::App::Create();
#endif  // defined(__ANDROID__)

  LogMessage("Initialized Firebase App.");

  LogMessage("Initialize Firebase Auth and Cloud Storage.");

  // Use ModuleInitializer to initialize both Auth and Storage, ensuring no
  // dependencies are missing.
  ::firebase::storage::Storage* storage = nullptr;
  ::firebase::auth::Auth* auth = nullptr;
  void* initialize_targets[] = {&auth, &storage};

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
        LogMessage("Attempt to initialize Cloud Storage.");
        void** targets = reinterpret_cast<void**>(data);
        ::firebase::InitResult result;
        firebase::storage::Storage* storage =
            firebase::storage::Storage::GetInstance(app, kStorageUrl, &result);
        *reinterpret_cast<::firebase::storage::Storage**>(targets[1]) = storage;
        LogMessage("Initialized storage with URL %s, %s",
                   kStorageUrl ? kStorageUrl : "(null)",
                   storage->url().c_str());
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
  LogMessage("Successfully initialized Firebase Auth and Cloud Storage.");

  // Sign in using Auth before accessing Storage.
  // The default Storage permissions allow anonymous users access. This will
  // work as long as your project's Authentication permissions allow anonymous
  // signin.
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
          "  Attempting to connect to Cloud Storage anyway. This may fail "
          "depending on the security settings.");
    }
  }

  // Generate a folder for the test data based on the time in milliseconds.
  int64_t time_in_microseconds = GetCurrentTimeInMicroseconds();

  char buffer[21] = {0};
  snprintf(buffer, sizeof(buffer), "%lld",
           static_cast<long long>(time_in_microseconds));  // NOLINT
  std::string saved_url = buffer;

  // Create a unique child in the storage that we can run in.
  firebase::storage::StorageReference ref;
  ref = storage->GetReference("test_app_data").Child(saved_url);

  LogMessage("Storage URL: gs://%s%s", ref.bucket().c_str(),
             ref.full_path().c_str());

  // Read and write from memory. This will save a small file and then read it
  // back from the storage to confirm that it was uploaded. Then it will remove
  // the file.
  {
    const std::string kSimpleTestFile =
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do "
        "eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim "
        "ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut "
        "aliquip ex ea commodo consequat. Duis aute irure dolor in "
        "reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla "
        "pariatur. Excepteur sint occaecat cupidatat non proident, sunt in "
        "culpa qui officia deserunt mollit anim id est laborum.";
    {
      LogMessage("Write a sample file.");
      std::string custom_metadata_key = "specialkey";
      std::string custom_metadata_value = "secret value";
      firebase::storage::Metadata metadata;
      metadata.set_content_type("test/plain");
      (*metadata.custom_metadata())[custom_metadata_key] =
          custom_metadata_value;
      firebase::Future<firebase::storage::Metadata> future =
          ref.Child("TestFile")
              .Child("SampleFile.txt")
              .PutBytes(&kSimpleTestFile[0], kSimpleTestFile.size(), metadata);
      WaitForCompletion(future, "Write");
      if (future.error() == 0) {
        if (future.result()->size_bytes() != kSimpleTestFile.size()) {
          LogMessage("ERROR: Incorrect number of bytes uploaded.");
        }
      }
    }

    {
      LogMessage("Read back the sample file.");

      const size_t kBufferSize = 1024;
      char buffer[kBufferSize];

      firebase::Future<size_t> future = ref.Child("TestFile")
                                            .Child("SampleFile.txt")
                                            .GetBytes(buffer, kBufferSize);
      WaitForCompletion(future, "Read");
      if (future.error() == 0) {
        if (*future.result() != kSimpleTestFile.size()) {
          LogMessage("ERROR: Incorrect number of bytes uploaded.");
        }
        if (memcmp(&kSimpleTestFile[0], buffer, kSimpleTestFile.size()) != 0) {
          LogMessage("ERROR: file contents did not match.");
        }
      }
    }
    {
      LogMessage("Delete the sample file.");

      firebase::Future<void> future =
          ref.Child("TestFile").Child("SampleFile.txt").Delete();
      WaitForCompletion(future, "Delete");
    }
  }

  LogMessage("Shutdown the Storage library.");
  delete storage;
  storage = nullptr;

  LogMessage("Signing out from anonymous account.");
  auth->SignOut();

  LogMessage("Shutdown the Auth library.");
  delete auth;
  auth = nullptr;

  LogMessage("Shutdown Firebase App.");
  delete app;
  app = nullptr;

  // Wait until the user wants to quit the app.
  while (!ProcessEvents(1000)) {
  }

  return 0;
}
