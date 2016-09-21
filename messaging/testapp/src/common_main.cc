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
#include "firebase/util.h"

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
    if (!message.error.empty()) LogMessage("error: %s", message.error.c_str());
    if (!message.message_id.empty()) {
      LogMessage("message_id: %s", message.message_id.c_str());
    }
    if (!message.data.empty()) {
      LogMessage("data:");
      for (const auto& field : message.data) {
        LogMessage("  %s: %s", field.first.c_str(), field.second.c_str());
      }
    }
    if (message.notification) {
      LogMessage("notification:");
      if (!message.notification->title.empty()) {
        LogMessage("  title: %s", message.notification->title.c_str());
      }
      if (!message.notification->body.empty()) {
        LogMessage("  body: %s", message.notification->body.c_str());
      }
      if (!message.notification->icon.empty()) {
        LogMessage("  icon: %s", message.notification->icon.c_str());
      }
      if (!message.notification->tag.empty()) {
        LogMessage("  tag: %s", message.notification->tag.c_str());
      }
      if (!message.notification->color.empty()) {
        LogMessage("  color: %s", message.notification->color.c_str());
      }
      if (!message.notification->sound.empty()) {
        LogMessage("  sound: %s", message.notification->sound.c_str());
      }
      if (!message.notification->click_action.empty()) {
        LogMessage("  click_action: %s",
                   message.notification->click_action.c_str());
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

#if defined(__ANDROID__)
  app = ::firebase::App::Create(::firebase::AppOptions(), GetJniEnv(),
                                GetActivity());
#else
  app = ::firebase::App::Create(::firebase::AppOptions());
#endif  // defined(__ANDROID__)

  LogMessage("Initialized Firebase App.");

  LogMessage("Initialize the Messaging library");

  ::firebase::ModuleInitializer initializer;
  initializer.Initialize(app, nullptr, [](::firebase::App* app, void*) {
    LogMessage("Try to initialize Firebase Messaging");
    return ::firebase::messaging::Initialize(*app, &g_listener);
  });
  while (initializer.InitializeLastResult().Status() !=
         firebase::kFutureStatusComplete) {
    if (ProcessEvents(100)) return 1;  // exit if requested
  }
  if (initializer.InitializeLastResult().Error() != 0) {
    LogMessage("Failed to initialize Firebase Messaging: %s",
               initializer.InitializeLastResult().ErrorMessage());
    ProcessEvents(2000);
    return 1;
  }

  LogMessage("Initialized Firebase Cloud Messaging.");

  ::firebase::messaging::Subscribe("TestTopic");
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
