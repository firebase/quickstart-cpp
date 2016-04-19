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

// Thin OS abstraction layer.
#include "main.h"  // NOLINT

::firebase::App* g_app;
::firebase::invites::InvitesSender* g_sender;
::firebase::invites::InvitesReceiver* g_receiver;

// Flush pending events for the main thread.
void process_events() {
#if defined(__ANDROID__)
  ProcessEvents(10);
#endif  // defined(__ANDROID__)
}

// Execute all methods of the C++ Invites API.
extern "C" int common_main(int argc, const char* argv[]) {
  LogMessage("Initializing Firebase App");

#if defined(__ANDROID__)
  g_app = ::firebase::App::Create(::firebase::AppOptions(FIREBASE_TESTAPP_NAME),
                                  GetJniEnv(), GetActivity());
#else
  g_app =
      ::firebase::App::Create(::firebase::AppOptions(FIREBASE_TESTAPP_NAME));
#endif  // defined(__ANDROID__)

  LogMessage("Created the Firebase App %x",
             static_cast<int>(reinterpret_cast<intptr_t>(g_app)));

  LogMessage("Creating an InvitesReceiver");
  g_receiver = new firebase::invites::InvitesReceiver(*g_app);

  LogMessage("Creating an InvitesSender");
  g_sender = new firebase::invites::InvitesSender(*g_app);

  bool testing_conversion = false;
  {
    // Check for received invitations.
    LogMessage("Fetch: Fetching invites...");
    auto future_result = g_receiver->Fetch();
    while (future_result.Status() == firebase::kFutureStatus_Pending) {
      process_events();
    }

    if (future_result.Status() == firebase::kFutureStatus_Invalid) {
      LogMessage("Fetch: Invalid, sorry!");
    } else if (future_result.Status() == firebase::kFutureStatus_Complete) {
      LogMessage("Fetch: Complete!");
      auto result = *future_result.Result();
      if (result.error_code != 0) {
        LogMessage("Fetch: Error %d: %s", result.error_code,
                   result.error_message.c_str());
      } else {
        // error_code == 0
        if (result.invitation_id != "") {
          LogMessage("Fetch: Got invitation ID: %s",
                     result.invitation_id.c_str());

          // We got an invitation ID, so let's try and convert it.
          LogMessage("ConvertInvitation: Converting invitation %s",
                     result.invitation_id.c_str());

          g_receiver->ConvertInvitation(result.invitation_id.c_str());
          testing_conversion = true;
        }
        if (result.deep_link != "") {
          LogMessage("Fetch: Got deep link: %s", result.deep_link.c_str());
        }
        if (result.invitation_id == "" && result.deep_link == "") {
          LogMessage("Fetch: No invitation ID or deep link, confirmed.");
        }
      }
    }
  }

  // Check if we are performing a conversion.
  if (testing_conversion) {
    auto future_result = g_receiver->ConvertInvitationLastResult();
    while (future_result.Status() == firebase::kFutureStatus_Pending) {
      process_events();
    }

    if (future_result.Status() == firebase::kFutureStatus_Invalid) {
      LogMessage("ConvertInvitation: Invalid, sorry!");
    } else if (future_result.Status() == firebase::kFutureStatus_Complete) {
      LogMessage("ConvertInvitation: Complete!");
      auto result = *future_result.Result();
      if (result.error_code != 0) {
        LogMessage("ConvertInvitation: Error %d: %s", result.error_code,
                   result.error_message.c_str());
      } else {
        LogMessage(
            "ConvertInvitation: Successfully converted invitation ID: %s",
            result.invitation_id.c_str());
      }
    }
  }

  {
    // Now try sending an invitation.
    LogMessage("SendInvite: Sending an invitation...");
    g_sender->SetTitleText("Invites Test App");
    g_sender->SetMessageText("Please try my app! It's awesome.");
    g_sender->SetCallToActionText("Download it for FREE");
    g_sender->SetDeepLinkUrl("http://google.com/abc");

    auto future_result = g_sender->SendInvite();
    while (future_result.Status() == firebase::kFutureStatus_Pending) {
      process_events();
    }

    if (future_result.Status() == firebase::kFutureStatus_Invalid) {
      LogMessage("SendInvite: Invalid, sorry!");
    } else if (future_result.Status() == firebase::kFutureStatus_Complete) {
      LogMessage("SendInvite: Complete!");
      auto result = *future_result.Result();
      if (result.error_code != 0) {
        LogMessage("SendInvite: Error %d: %s", result.error_code,
                   result.error_message.c_str());
      } else {
        // error_code == 0
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

  LogMessage("Sample finished.");
  delete g_sender;
  g_sender = nullptr;
  delete g_receiver;
  g_receiver = nullptr;

  return 0;
}
