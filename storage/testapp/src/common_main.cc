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

const char* kPutFileTestFile = "PutFileTest.txt";
const char* kGetFileTestFile = "GetFileTest.txt";

// Optionally set this to your Cloud Storage URL (gs://...) to test
// in a specific Cloud Storage bucket.
const char* kStorageUrl = nullptr;

class StorageListener : public firebase::storage::Listener {
 public:
  StorageListener()
      : on_paused_was_called_(false), on_progress_was_called_(false) {}

  // Tracks whether OnPaused was ever called
  void OnPaused(firebase::storage::Controller* controller) override {
    (void)controller;
    on_paused_was_called_ = true;
  }

  void OnProgress(firebase::storage::Controller* controller) override {
    (void)controller;
    on_progress_was_called_ = true;
  }

  bool on_paused_was_called() const { return on_paused_was_called_; }
  bool on_progress_was_called() const { return on_progress_was_called_; }

 public:
  bool on_paused_was_called_;
  bool on_progress_was_called_;
};

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
  app = ::firebase::App::Create(GetJniEnv(), GetActivity());
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
        *reinterpret_cast<::firebase::storage::Storage**>(targets[1]) =
            ::firebase::storage::Storage::GetInstance(app, kStorageUrl,
                                                      &result);
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

  // Create a unique child in the storage that we can run our tests in.
  firebase::storage::StorageReference ref;
  ref = storage->GetReference("test_app_data").Child(saved_url);

  firebase::storage::Metadata test_metadata;

  LogMessage("Storage URL: gs://%s/%s", ref.bucket().c_str(),
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
      LogMessage("TEST: Write a sample file from byte buffer.");
      firebase::Future<firebase::storage::Metadata> future =
          ref.Child("TestFile")
              .Child("File1.txt")
              .PutBytes(&kSimpleTestFile[0], kSimpleTestFile.size());
      WaitForCompletion(future, "Write Bytes");
      if (future.error() != firebase::storage::kErrorNone) {
        LogMessage("ERROR: Write sample file failed.");
        LogMessage("  File1.txt: Error %d: %s", future.error(),
                   future.error_message());
      } else {
        LogMessage("SUCCESS: Wrote file with PutBytes.");
        auto metadata = future.result();
        if (metadata->size_bytes() == kSimpleTestFile.size()) {
          LogMessage("SUCCESS: Metadata reports correct size.");
        } else {
          LogMessage("ERROR: Metadata reports incorrect size.");
          LogMessage("  Got %i bytes, expected %i bytes.",
                     metadata->size_bytes(), kSimpleTestFile.size());
        }
      }
    }

    {
      LogMessage("TEST: Write a sample file from local file.");

      // Write file that we're going to upload.
      std::string path = PathForResource() + kPutFileTestFile;
      // Cloud Storage expects file:// in front of local paths.
      std::string file_path = "file://" + path;

      FILE* file = fopen(path.c_str(), "w");
      std::fwrite(kSimpleTestFile.c_str(), 1, kSimpleTestFile.size(), file);
      fclose(file);

      firebase::storage::Metadata new_metadata;
      new_metadata.set_content_type("text/html");
      new_metadata.custom_metadata()->insert(std::make_pair("hello", "world"));

      firebase::Future<firebase::storage::Metadata> future =
          ref.Child("TestFile")
              .Child("File2.txt")
              .PutFile(file_path.c_str(), new_metadata);
      WaitForCompletion(future, "Write File");
      if (future.error() != firebase::storage::kErrorNone) {
        LogMessage("ERROR: Write file failed.");
        LogMessage("  File2.txt: Error %d: %s", future.error(),
                   future.error_message());
      } else {
        LogMessage("SUCCESS: Wrote file with PutFile.");
        auto metadata = future.result();
        if (metadata->size_bytes() == kSimpleTestFile.size()) {
          LogMessage("SUCCESS: Metadata reports correct size.");
        } else {
          LogMessage("ERROR: Metadata reports incorrect size.");
          LogMessage("  Got %i bytes, expected %i bytes.",
                     metadata->size_bytes(), kSimpleTestFile.size());
        }
        if (strcmp(metadata->content_type(), new_metadata.content_type()) ==
            0) {
          LogMessage(
              "SUCCESS: Metadata has correct content type set at upload.");
        } else {
          LogMessage(
              "ERROR: Metadata has incorrect content type set at upload.");
          LogMessage("  Got %s, expected %s.", metadata->content_type(),
                     new_metadata.content_type());
        }
        auto pair1 = metadata->custom_metadata()->find("hello");
        if (pair1 != metadata->custom_metadata()->end() &&
            pair1->second == "world") {
          LogMessage(
              "SUCCESS: Metadata has correct custom metadata set at upload.");
        } else {
          LogMessage(
              "SUCCESS: Metadata has incorrect custom metadata set at upload.");
        }
      }
    }

    // Get the values that we just set, and confirm that they match what we
    // set them to.
    {
      LogMessage("TEST: Read a sample file with GetBytes.");

      // Read the storage file to byte buffer.
      {
        const size_t kBufferSize = 1024;
        char buffer[kBufferSize];

        firebase::Future<size_t> future = ref.Child("TestFile")
                                              .Child("File1.txt")
                                              .GetBytes(buffer, kBufferSize);
        WaitForCompletion(future, "Read Bytes");

        // Check if the file contents is correct.
        if (future.error() == firebase::storage::kErrorNone) {
          if (*future.result() != kSimpleTestFile.size()) {
            LogMessage(
                "ERROR: Read file failed, read incorrect number of bytes (read "
                "%z, expected %z)",
                *future.result(), kSimpleTestFile.size());
          } else if (memcmp(&kSimpleTestFile[0], buffer,
                            kSimpleTestFile.size()) == 0) {
            LogMessage("SUCCESS: Read file succeeded.");
          } else {
            LogMessage("ERROR: Read file failed, file contents did not match.");
          }
        } else {
          LogMessage("ERROR: Read file failed.");
        }
      }

      LogMessage("TEST: Read a sample file with GetFile.");

      // Read the storage file to local file.
      {
        const size_t kBufferSize = 1024;
        char buffer[kBufferSize];

        // Write file that we're going to upload.
        std::string path = PathForResource() + kGetFileTestFile;
        // Cloud Storage expects file:// in front of local paths.
        std::string file_path = "file://" + path;

        firebase::Future<size_t> future =
            ref.Child("TestFile").Child("File2.txt").GetFile(file_path.c_str());
        WaitForCompletion(future, "Read File");

        FILE* file = fopen(path.c_str(), "r");
        std::fread(buffer, 1, kBufferSize, file);
        fclose(file);

        // Check if the file contents is correct.
        if (future.error() == firebase::storage::kErrorNone) {
          if (*future.result() != kSimpleTestFile.size()) {
            LogMessage(
                "ERROR: Read file failed, read incorrect number of bytes (read "
                "%z, expected %z)",
                *future.result(), kSimpleTestFile.size());
          } else if (memcmp(&kSimpleTestFile[0], buffer,
                            kSimpleTestFile.size()) == 0) {
            LogMessage("SUCCESS: Read file succeeded.");
          } else {
            LogMessage("ERROR: Read file failed, file contents did not match.");
          }
        } else {
          LogMessage("ERROR: Read file failed.");
        }
      }

      // Check if the timestamp is correct.
      {
        LogMessage("TEST: Check sample file metadata.");

        firebase::Future<firebase::storage::Metadata> future =
            ref.Child("TestFile").Child("File1.txt").GetMetadata();
        WaitForCompletion(future, "GetFileMetadata");
        const firebase::storage::Metadata* metadata = future.result();
        if (future.error() == firebase::storage::kErrorNone) {
          test_metadata = *metadata;  // Save a copy of the metadata for later.

          // Get the current time to compare to the Timestamp.
          int64_t current_time_seconds = static_cast<int64_t>(time(nullptr));
          int64_t updated_time_milliseconds = metadata->updated_time();
          int64_t updated_time_seconds = updated_time_milliseconds / 1000;
          int64_t time_difference_seconds =
              updated_time_seconds - current_time_seconds;
          // As long as our timestamp is within a day, it's correct enough for
          // our purposes.
          const int64_t kAllowedTimeDifferenceSeconds = 60L * 60L * 24L;
          if (time_difference_seconds > kAllowedTimeDifferenceSeconds ||
              time_difference_seconds < -kAllowedTimeDifferenceSeconds) {
            LogMessage("ERROR: Incorrect metadata.");
            LogMessage("  Timestamp: Got %lld, expected something near %lld",
                       updated_time_seconds, current_time_seconds);
          } else {
            LogMessage("SUCCESS: Read file successfully.");
          }
        } else {
          LogMessage("ERROR: Read file failed.");
        }
        // Check custom metadata field to make sure it is empty.
        auto custom_metadata = metadata->custom_metadata();
        if (!custom_metadata->empty()) {
          LogMessage("ERROR: Metadata reports incorrect custom metadata.");
        } else {
          LogMessage("SUCCESS: Metadata reports correct custom metadata.");
        }

        // Add some values to custom metadata, update and then compare.
        custom_metadata->insert(std::make_pair("Key", "Value"));
        custom_metadata->insert(std::make_pair("Foo", "Bar"));
        firebase::Future<firebase::storage::Metadata> custom_metadata_future =
            ref.Child("TestFile").Child("File1.txt").UpdateMetadata(*metadata);
        WaitForCompletion(custom_metadata_future, "UpdateMetadata");
        if (future.error() != firebase::storage::kErrorNone) {
          LogMessage("ERROR: UpdateMetadata failed.");
          LogMessage("  File1.txt: Error %d: %s", future.error(),
                     future.error_message());
        } else {
          LogMessage("SUCCESS: Updated Metadata.");
          const firebase::storage::Metadata* new_metadata =
              custom_metadata_future.result();
          auto new_custom_metadata = new_metadata->custom_metadata();
          auto pair1 = new_custom_metadata->find("Key");
          auto pair2 = new_custom_metadata->find("Foo");
          if (pair1 == new_custom_metadata->end() || pair1->second != "Value" ||
              pair2 == new_custom_metadata->end() || pair2->second != "Bar") {
            LogMessage(
                "ERROR: New metadata reports incorrect custom metadata.");
          } else {
            LogMessage(
                "SUCCESS: New metadata reports correct custom metadata.");
          }
        }
      }

      // Check the download URL.
      {
        LogMessage("TEST: Check for a download URL.");
        firebase::Future<std::string> future =
            ref.Child("TestFile").Child("File1.txt").GetDownloadUrl();

        WaitForCompletion(future, "GetDownloadUrl");
        if (future.error() != firebase::storage::kErrorNone) {
          LogMessage("ERROR: Couldn't get download URL.");
          LogMessage("  File1.txt: Error %d: %s", future.error(),
                     future.error_message());
        } else {
          LogMessage("SUCCESS: Got URL: ");
          const std::string* download_url = future.result();
          LogMessage("  %s", download_url->c_str());
        }
      }
      {
        firebase::Future<firebase::storage::Metadata> future =
            ref.Child("TestFile").Child("File1.txt").GetMetadata();
        WaitForCompletion(future, "GetFileMetadataForDownloadUrl");
        if (future.error() == firebase::storage::kErrorNone) {
          if (future.result()->download_url() != nullptr) {
            LogMessage("SUCCESS: Got URL in metadata: %s",
                       future.result()->download_url());
          } else {
            LogMessage("ERROR: No download URL listed in metadata.");
          }
        } else {
          LogMessage("ERROR: Couldn't read metadata to check download URL.");
        }
      }

      // Try removing the file.
      {
        // Call delete on file.
        LogMessage("TEST: Removing file.");
        firebase::Future<void> delete_future =
            ref.Child("TestFile").Child("File1.txt").Delete();
        WaitForCompletion(delete_future, "DeleteFile");
        if (delete_future.error() == firebase::storage::kErrorNone) {
          LogMessage("SUCCESS: File was removed.");
        } else {
          LogMessage("ERROR: File was not removed.");
        }

        // Verify it can no longer be read.
        const size_t kBufferSize = 1024;
        char buffer[kBufferSize];
        firebase::Future<size_t> read_future =
            ref.Child("TestFile")
                .Child("File1.txt")
                .GetBytes(buffer, kBufferSize);
        while (read_future.status() == firebase::kFutureStatusPending) {
          ProcessEvents(100);
        }
        if (read_future.error() == firebase::storage::kErrorObjectNotFound) {
          LogMessage("SUCCESS: File could not be read, as expected.");
        } else {
          LogMessage("ERROR: File could be read after removal. Status = %d: %s",
                     read_future.error(), read_future.error_message());
        }
      }
    }
  }

  {
    const size_t kLargeFileSize = 2 * 1024 * 1024;
    std::vector<char> large_test_file;
    large_test_file.resize(kLargeFileSize);
    for (int i = 0; i < kLargeFileSize; ++i) {
      // Fill the buffer with the alphabet.
      large_test_file[i] = 'a' + (i % 26);
    }
    const std::vector<char>& kLargeTestFile = large_test_file;
    bool wrote_file = false;

    {
      LogMessage("TEST: Write a large file, pause, and resume mid-way.");
      StorageListener listener;
      firebase::storage::Controller controller;
      firebase::Future<firebase::storage::Metadata> future =
          ref.Child("TestFile")
              .Child("File3.txt")
              .PutBytes(&kLargeTestFile[0], kLargeFileSize, &listener,
                        &controller);

      // Ensure the Controller is valid now that we have associated it with an
      // operation.
      if (!controller.is_valid()) {
        LogMessage("ERROR: Controller was invalid.");
      }

      ProcessEvents(500);
      // After waiting a moment for the operation to start, pause the operation
      // and verify it was successfully paused. Note that pause might not take
      // effect immediately, so we give it a few moments to pause before
      // failing.
      LogMessage("INFO: Pausing.");
      if (controller.Pause()) {
        ProcessEvents(5000);
        if (!listener.on_paused_was_called()) {
          LogMessage("ERROR: Listener OnPaused callback was not called.");
        }
        LogMessage("INFO: Resuming.");
        if (!controller.Resume()) {
          LogMessage("ERROR: Resume() failed.");
        }
      } else {
        LogMessage("ERROR: Pause() failed.");
      }

      WaitForCompletion(future, "WriteLargeFile");

      // Ensure the progress callback was called.
      if (!listener.on_progress_was_called()) {
        LogMessage("ERROR: Listener OnProgress callback was not called.");
      }

      if (future.error() != firebase::storage::kErrorNone) {
        LogMessage("ERROR: Write file failed.");
        LogMessage("  TestFile: Error %d: %s", future.error(),
                   future.error_message());
      } else {
        LogMessage("SUCCESS: Wrote large file.");
        wrote_file = true;
        auto metadata = future.result();
        if (metadata->size_bytes() == kLargeFileSize) {
          LogMessage("SUCCESS: Metadata reports correct size.");
        } else {
          LogMessage("ERROR: Metadata reports incorrect size.");
          LogMessage("  Got %i bytes, expected %i bytes.",
                     metadata->size_bytes(), kLargeFileSize);
        }
      }
    }

    if (wrote_file) {
      LogMessage("TEST: Reading previously-uploaded large file.");

      std::vector<char> buffer;
      buffer.resize(kLargeFileSize);
      StorageListener listener;
      firebase::storage::Controller controller;
      firebase::Future<size_t> future =
          ref.Child("TestFile")
              .Child("File3.txt")
              .GetBytes(&buffer[0], buffer.size(), &listener, &controller);

      // Ensure the Controller is valid now that we have associated it with an
      // operation.
      if (!controller.is_valid()) {
        LogMessage("ERROR: Controller was invalid.");
      }

      WaitForCompletion(future, "ReadLargeFile");

      // Ensure the progress callback was called.
      if (!listener.on_progress_was_called()) {
        LogMessage("ERROR: Listener OnProgress callback was not called.");
      }

      // Check if the file contents is correct.
      if (future.error() == firebase::storage::kErrorNone) {
        if (*future.result() != kLargeFileSize) {
          LogMessage(
              "ERROR: Read file failed, read incorrect number of bytes (read "
              "%d, expected %d)",
              static_cast<int>(*future.result()),
              static_cast<int>(kLargeFileSize));
        } else if (std::memcmp(&kLargeTestFile[0], &buffer[0],
                               kLargeFileSize) == 0) {
          LogMessage("SUCCESS: Read file succeeded.");
        } else {
          LogMessage("ERROR: Read file failed, file contents did not match.");
        }
      } else {
        LogMessage("ERROR: Read file failed.");
      }
    }
    {
      LogMessage("TEST: Write a large file and cancel mid-way.");
      firebase::storage::Controller controller;
      firebase::Future<firebase::storage::Metadata> future =
          ref.Child("TestFile")
              .Child("File4.txt")
              .PutBytes(&kLargeTestFile[0], kLargeFileSize, nullptr,
                        &controller);

      // Ensure the Controller is valid now that we have associated it with an
      // operation.
      if (!controller.is_valid()) {
        LogMessage("ERROR: Controller was invalid.");
      }

      // Cancel the operation and verify it was successfully canceled.
      controller.Cancel();

      while (future.status() == firebase::kFutureStatusPending) {
        ProcessEvents(100);
      }
      if (future.error() != firebase::storage::kErrorCancelled) {
        LogMessage("ERROR: Write cancellation failed.");
        LogMessage("  TestFile: Error %d: %s", future.error(),
                   future.error_message());
      } else {
        LogMessage("SUCCESS: Canceled file upload.");
      }
    }
  }

  // Check if test_metadata started valid and was then invalidated.
  bool test_metadata_was_valid = test_metadata.is_valid();

  LogMessage("Shutdown the Storage library.");
  delete storage;
  storage = nullptr;

  // Ensure that the ref we had is now invalid.
  if (!ref.is_valid()) {
    LogMessage("SUCCESS: Reference was invalidated on library shutdown.");
  } else {
    LogMessage("ERROR: Reference is still valid after library shutdown.");
  }

  if (test_metadata_was_valid) {
    if (!test_metadata.is_valid()) {
      LogMessage("SUCCESS: Metadata was invalidated on library shutdown.");
    } else {
      LogMessage("ERROR: Metadata is still valid after library shutdown.");
    }
  } else {
    LogMessage(
        "WARNING: Metadata was already invalid at shutdown, couldn't check.");
  }

  LogMessage("Signing out from anonymous account.");
  auth->SignOut();
  LogMessage("Shutdown the Auth library.");
  delete auth;
  auth = nullptr;

  LogMessage("Shutdown Firebase App.");
  delete app;

  // Wait until the user wants to quit the app.
  while (!ProcessEvents(1000)) {
  }

  return 0;
}
