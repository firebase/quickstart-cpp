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
#include "firebase/messaging.h"
#if defined(__ANDROID__)
#include "google_play_services/availability.h"
#endif  // defined(__ANDROID__)

// Thin OS abstraction layer.
#include "main.h"  // NOLINT

class MessageListener : public firebase::messaging::Listener {
 public:
  virtual void OnMessage(const ::firebase::messaging::Message& message) {
    // When messages are received by the server, they are placed into an
    // internal queue, waiting to be consumed. When ProcessMessages is called,
    // this OnMessage function is called once for each queued message.
    LogMessage("Recieved a new message");
    if (!message.from.empty()) LogMessage("from: %s", message.from.c_str());
    if (!message.data.empty()) {
      LogMessage("data:");
      typedef std::map<std::string, std::string>::const_iterator MapIter;
      for (MapIter it = message.data.begin(); it != message.data.end(); ++it) {
        LogMessage("  %s: %s", it->first.c_str(), it->second.c_str());
      }
    }
  }

  virtual void OnTokenReceived(const char* token) {
    // To send a message to a specific instance of your app a registration token
    // is required. These tokens are unique for each instance of the app. When
    // messaging::Initialize is called, a request is sent to the Firebase Cloud
    // Messaging server to generate a token. When that token is ready,
    // OnTokenReceived will be called. The token should be cached locally so
    // that a request doesn't need to be generated each time the app is started.
    //
    // Once a token is generated is should be sent to your app server, which can
    // then use it to send messages to users.
    LogMessage("Recieved Registration Token: %s", token);
  }
};

MessageListener g_listener;

// Execute all methods of the C++ Firebase Cloud Messaging API.
extern "C" int common_main(int argc, const char* argv[]) {
  ::firebase::App* app;

  LogMessage("Initialize the Messaging library");

#if defined(__ANDROID__)
  app = ::firebase::App::Create(::firebase::AppOptions(), GetJniEnv(),
                                GetActivity());
#else
  app = ::firebase::App::Create(::firebase::AppOptions());
#endif  // defined(__ANDROID__)

  LogMessage("Initialized Firebase App.");

  ::firebase::InitResult init_result;
  bool try_again;
  do {
    try_again = false;
    init_result = ::firebase::messaging::Initialize(*app, &g_listener);

#if defined(__ANDROID__)
    // On Android, we need to update or activate Google Play services
    // before we can initialize this Firebase module.
    if (init_result == firebase::kInitResultFailedMissingDependency) {
      LogMessage("Google Play services unavailable, trying to fix.");
      firebase::Future<void> make_available =
          google_play_services::MakeAvailable(app->GetJNIEnv(),
                                              app->activity());
      while (make_available.Status() != ::firebase::kFutureStatusComplete) {
        if (ProcessEvents(100)) return 1;  // Return if exit was triggered.
      }

      if (make_available.Error() == 0) {
        LogMessage("Google Play services now available, continuing.");
        try_again = true;
      } else {
        LogMessage("Google Play services still unavailable.");
      }
    }
#endif  // defined(__ANDROID__)
  } while (try_again);

  if (init_result != ::firebase::kInitResultSuccess) {
    LogMessage("Failed to initialized Firebase Cloud Messaging, exiting.");
    ProcessEvents(2000);
    return 1;
  }
  LogMessage("Initialized Firebase Cloud Messaging.");

  ::firebase::messaging::Subscribe("/topics/TestTopic");
  LogMessage("Subscribed to TestTopic");

  bool done = false;
  while (!done) {
    // Process events so that the client doesn't hang.
    done = ProcessEvents(1000);
  }
  ::firebase::messaging::Terminate();
  delete app;

  return 0;
}
