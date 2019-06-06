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
#include "firebase/future.h"
#include "firebase/messaging.h"
#include "firebase/util.h"

// Thin OS abstraction layer.
#include "main.h"  // NOLINT

// Don't return until `future` is complete.
// Print a message for whether the result mathes our expectations.
// Returns true if the application should exit.
static bool WaitForFuture(const ::firebase::FutureBase& future, const char* fn,
                          ::firebase::messaging::Error expected_error,
                          bool log_error = true) {
  // Note if the future has not be started properly.
  if (future.status() == ::firebase::kFutureStatusInvalid) {
    LogMessage("ERROR: Future for %s is invalid", fn);
    return false;
  }

  // Wait for future to complete.
  LogMessage("  %s...", fn);
  while (future.status() == ::firebase::kFutureStatusPending) {
    if (ProcessEvents(100)) return true;
  }

  // Log error result.
  if (log_error) {
    const ::firebase::messaging::Error error =
        static_cast<::firebase::messaging::Error>(future.error());
    if (error == expected_error) {
      const char* error_message = future.error_message();
      if (error_message) {
        LogMessage("%s completed as expected", fn);
      } else {
        LogMessage("%s completed as expected, error: %d '%s'", fn, error,
                   error_message);
      }
    } else {
      LogMessage("ERROR: %s completed with error: %d, `%s`", fn, error,
                 future.error_message());
    }
  }
  return false;
}

// Execute all methods of the C++ Firebase Cloud Messaging API.
extern "C" int common_main(int argc, const char* argv[]) {
  ::firebase::App* app;
  ::firebase::messaging::PollableListener listener;

#if defined(__ANDROID__)
  app = ::firebase::App::Create(GetJniEnv(), GetActivity());
#else
  app = ::firebase::App::Create();
#endif  // defined(__ANDROID__)

  LogMessage("Initialized Firebase App.");

  LogMessage("Initialize the Messaging library");

  ::firebase::ModuleInitializer initializer;
  initializer.Initialize(
      app, &listener, [](::firebase::App* app, void* userdata) {
        LogMessage("Try to initialize Firebase Messaging");
        ::firebase::messaging::PollableListener* listener =
            static_cast<::firebase::messaging::PollableListener*>(userdata);
        firebase::messaging::MessagingOptions options;
        // Prevent the app from requesting permission to show notifications
        // immediately upon starting up. Since it the prompt is being
        // suppressed, we must manually display it with a call to
        // RequestPermission() elsewhere.
        options.suppress_notification_permission_prompt = true;

        return ::firebase::messaging::Initialize(*app, listener, options);
      });

  while (initializer.InitializeLastResult().status() !=
         firebase::kFutureStatusComplete) {
    if (ProcessEvents(100)) return 1;  // exit if requested
  }
  if (initializer.InitializeLastResult().error() != 0) {
    LogMessage("Failed to initialize Firebase Messaging: %s",
               initializer.InitializeLastResult().error_message());
    ProcessEvents(2000);
    return 1;
  }

  LogMessage("Initialized Firebase Cloud Messaging.");

  // This will display the prompt to request permission to receive notifications
  // if the prompt has not already been displayed before. (If the user already
  // responded to the prompt, their decision is cached by the OS and can be
  // changed in the OS settings).
  ::firebase::Future<void> result = ::firebase::messaging::RequestPermission();
  LogMessage("Display permission prompt if necessary.");
  while (result.status() == ::firebase::kFutureStatusPending) {
    ProcessEvents(100);
  }
  if (result.error() ==
      ::firebase::messaging::kErrorFailedToRegisterForRemoteNotifications) {
    LogMessage("Error registering for remote notifications.");
  } else {
    LogMessage("Finished checking for permission.");
  }

  // Subscribe to topics.
  WaitForFuture(::firebase::messaging::Subscribe("TestTopic"),
                "::firebase::messaging::Subscribe(\"TestTopic\")",
                ::firebase::messaging::kErrorNone);
  WaitForFuture(::firebase::messaging::Subscribe("!@#$%^&*()"),
                "::firebase::messaging::Subscribe(\"!@#$%^&*()\")",
                ::firebase::messaging::kErrorInvalidTopicName);

  bool done = false;
  while (!done) {
    std::string token;
    if (listener.PollRegistrationToken(&token)) {
      LogMessage("Received Registration Token: %s", token.c_str());
    }

    ::firebase::messaging::Message message;
    while (listener.PollMessage(&message)) {
      LogMessage("Received a new message");
      LogMessage("This message was %s by the user",
                 message.notification_opened ? "opened" : "not opened");
      if (!message.from.empty()) LogMessage("from: %s", message.from.c_str());
      if (!message.error.empty())
        LogMessage("error: %s", message.error.c_str());
      if (!message.message_id.empty()) {
        LogMessage("message_id: %s", message.message_id.c_str());
      }
      if (!message.link.empty()) {
        LogMessage("  link: %s", message.link.c_str());
      }
      if (!message.data.empty()) {
        LogMessage("data:");
        for (const auto& field : message.data) {
          LogMessage("  %s: %s", field.first.c_str(), field.second.c_str());
        }
      }
      if (message.notification) {
        LogMessage("notification:");
        if (message.notification->android) {
          LogMessage("  android:");
          LogMessage("    channel_id: %s",
                     message.notification->android->channel_id.c_str());
        }
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
    // Process events so that the client doesn't hang.
    done = ProcessEvents(1000);
  }

  ::firebase::messaging::Terminate();
  delete app;

  return 0;
}
