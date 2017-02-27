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
#include "firebase/invites.h"
#include "firebase/util.h"

// Thin OS abstraction layer.
#include "main.h"  // NOLINT

void ConversionFinished(const firebase::Future<void>& future_result,
                        void* user_data) {
  if (future_result.status() == firebase::kFutureStatusInvalid) {
    LogMessage("ConvertInvitation: Invalid, sorry!");
  } else if (future_result.status() == firebase::kFutureStatusComplete) {
    LogMessage("ConvertInvitation: Complete!");
    if (future_result.error() != 0) {
      LogMessage("ConvertInvitation: Error %d: %s", future_result.error(),
                 future_result.error_message());
    } else {
      LogMessage("ConvertInvitation: Successfully converted invitation");
    }
  }
}

class InviteListener : public firebase::invites::Listener {
 public:
  void OnInviteReceived(const char* invitation_id, const char* deep_link,
                        bool is_strong_match) override {  // NOLINT
    if (invitation_id != nullptr) {
      LogMessage("InviteReceived: Got invitation ID: %s", invitation_id);

      // We got an invitation ID, so let's try and convert it.
      LogMessage("ConvertInvitation: Converting invitation %s", invitation_id);

      ::firebase::invites::ConvertInvitation(invitation_id)
          .OnCompletion(ConversionFinished, nullptr);
    }
    if (deep_link != nullptr) {
      LogMessage("InviteReceived: Got deep link: %s", deep_link);
    }
  }

  void OnInviteNotReceived() override {
    LogMessage("InviteReceived: No invitation ID or deep link, confirmed.");
  }

  void OnErrorReceived(int error_code, const char* error_message) override {
    LogMessage("Error (%d) on received invite: %s", error_code, error_message);
  }
};

InviteListener g_listener;

// Execute all methods of the C++ Invites API.
extern "C" int common_main(int argc, const char* argv[]) {
  ::firebase::App* app;

  LogMessage("Initializing Firebase App");

#if defined(__ANDROID__)
  app = ::firebase::App::Create(GetJniEnv(), GetActivity());
#else
  app = ::firebase::App::Create();
#endif  // defined(__ANDROID__)

  LogMessage("Created the Firebase App %x",
             static_cast<int>(reinterpret_cast<intptr_t>(app)));

  ::firebase::ModuleInitializer initializer;
  initializer.Initialize(app, nullptr, [](::firebase::App* app, void*) {
    LogMessage("Try to initialize Invites");
    return ::firebase::invites::Initialize(*app);
  });
  while (initializer.InitializeLastResult().status() !=
         firebase::kFutureStatusComplete) {
    if (ProcessEvents(100)) return 1;  // exit if requested
  }
  if (initializer.InitializeLastResult().error() != 0) {
    LogMessage("Failed to initialize Firebase Invites: %s",
               initializer.InitializeLastResult().error_message());
    ProcessEvents(2000);
    return 1;
  }
  LogMessage("Initialized Firebase Invites.");

  // First, try sending an Invite.
  {
    LogMessage("SendInvite: Sending an invitation...");
    ::firebase::invites::Invite invite;
    invite.title_text = "Invites Test App";
    invite.message_text = "Please try my app! It's awesome.";
    invite.call_to_action_text = "Download it for FREE";
    invite.deep_link_url = "http://google.com/abc";
    auto future_result = ::firebase::invites::SendInvite(invite);
    while (future_result.status() == firebase::kFutureStatusPending) {
      if (ProcessEvents(10)) break;
    }

    if (future_result.status() == firebase::kFutureStatusInvalid) {
      LogMessage("SendInvite: Invalid, sorry!");
    } else if (future_result.status() == firebase::kFutureStatusComplete) {
      LogMessage("SendInvite: Complete!");
      if (future_result.error() != 0) {
        LogMessage("SendInvite: Error %d: %s", future_result.error(),
                   future_result.error_message());
      } else {
        auto result = *future_result.result();
        // error == 0
        if (result.invitation_ids.size() == 0) {
          LogMessage("SendInvite: Nothing sent, user must have canceled.");
        } else {
          LogMessage("SendInvite: %d invites sent successfully.",
                     result.invitation_ids.size());
          for (int i = 0; i < result.invitation_ids.size(); i++) {
            LogMessage("SendInvite: Invite code: %s",
                       result.invitation_ids[i].c_str());
          }
        }
      }
    }
  }

  // Then, set the listener, which will check for any invitations.
  ::firebase::invites::SetListener(&g_listener);

  LogMessage("Listener set, main loop finished.");

  while (!ProcessEvents(1000)) {
  }

  ::firebase::invites::Terminate();
  delete app;
  app = nullptr;

  return 0;
}
