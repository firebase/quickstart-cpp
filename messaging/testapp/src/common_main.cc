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

#include <pthread.h>
#include <unistd.h>
#include "firebase/app.h"
#include "firebase/messaging.h"

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
  ::firebase::App* g_app;
#if defined(__ANDROID__)
  g_app = ::firebase::App::Create(::firebase::AppOptions(FIREBASE_TESTAPP_NAME),
                                  GetJniEnv(), GetActivity());
#else
  g_app =
      ::firebase::App::Create(::firebase::AppOptions(FIREBASE_TESTAPP_NAME));
#endif  // defined(__ANDROID__)

  LogMessage("Initialized Firebase App.");

  ::firebase::messaging::Initialize(*g_app, &g_listener);
  LogMessage("Initialized Firebase Cloud Messaging.");

  ::firebase::messaging::Subscribe("/topics/TestTopic");
  LogMessage("Subscribed to TestTopic");

#if defined(__ANDROID__)
  while (!ProcessEvents(1000)) {
    // Process events so that the client doesn't hang.
    LogMessage("Main tick");
  }
#endif  // defined(__ANDROID__)
  return 0;
}
