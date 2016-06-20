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

// Execute all methods of the C++ Invites API.
extern "C" int common_main(int argc, const char* argv[]) {
  ::firebase::App* app;
  ::firebase::invites::InvitesSender* sender;
  ::firebase::invites::InvitesReceiver* receiver;

  LogMessage("Initializing Firebase App");

#if defined(__ANDROID__)
  app = ::firebase::App::Create(::firebase::AppOptions(), GetJniEnv(),
                                GetActivity());
#else
  app = ::firebase::App::Create(::firebase::AppOptions());
#endif  // defined(__ANDROID__)

  if (app == nullptr) {
    LogMessage("Couldn't create firebase app, aborting.");
    // Wait until the user wants to quit the app.
    while (!ProcessEvents(1000)) {
    }
    return 1;
  }

  LogMessage("Created the Firebase App %x",
             static_cast<int>(reinterpret_cast<intptr_t>(app)));

  LogMessage("Creating an InvitesReceiver");
  receiver = new firebase::invites::InvitesReceiver(*app);

  LogMessage("Creating an InvitesSender");
  sender = new firebase::invites::InvitesSender(*app);

  bool testing_conversion = false;
  {
    // Check for received invitations.
    LogMessage("Fetch: Fetching invites...");
    auto future_result = receiver->Fetch();
    while (future_result.Status() == firebase::kFutureStatusPending) {
      if (ProcessEvents(10)) break;
    }

    if (future_result.Status() == firebase::kFutureStatusInvalid) {
      LogMessage("Fetch: Invalid, sorry!");
    } else if (future_result.Status() == firebase::kFutureStatusComplete) {
      LogMessage("Fetch: Complete!");
      if (future_result.Error() != 0) {
        LogMessage("Fetch: Error %d: %s", future_result.Error(),
                   future_result.ErrorMessage());
      } else {
        auto result = *future_result.Result();
        // error_code == 0
        if (result.invitation_id != "") {
          LogMessage("Fetch: Got invitation ID: %s",
                     result.invitation_id.c_str());

          // We got an invitation ID, so let's try and convert it.
          LogMessage("ConvertInvitation: Converting invitation %s",
                     result.invitation_id.c_str());

          receiver->ConvertInvitation(result.invitation_id.c_str());
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
    auto future_result = receiver->ConvertInvitationLastResult();
    while (future_result.Status() == firebase::kFutureStatusPending) {
      if (ProcessEvents(10)) break;
    }

    if (future_result.Status() == firebase::kFutureStatusInvalid) {
      LogMessage("ConvertInvitation: Invalid, sorry!");
    } else if (future_result.Status() == firebase::kFutureStatusComplete) {
      LogMessage("ConvertInvitation: Complete!");
      if (future_result.Error() != 0) {
        LogMessage("ConvertInvitation: Error %d: %s", future_result.Error(),
                   future_result.ErrorMessage());
      } else {
        auto result = *future_result.Result();
        LogMessage(
            "ConvertInvitation: Successfully converted invitation ID: %s",
            result.invitation_id.c_str());
      }
    }
  }

  {
    // Now try sending an invitation.
    LogMessage("SendInvite: Sending an invitation...");
    sender->SetTitleText("Invites Test App");
    sender->SetMessageText("Please try my app! It's awesome.");
    sender->SetCallToActionText("Download it for FREE");
    sender->SetDeepLinkUrl("http://google.com/abc");

    auto future_result = sender->SendInvite();
    while (future_result.Status() == firebase::kFutureStatusPending) {
      if (ProcessEvents(10)) break;
    }

    if (future_result.Status() == firebase::kFutureStatusInvalid) {
      LogMessage("SendInvite: Invalid, sorry!");
    } else if (future_result.Status() == firebase::kFutureStatusComplete) {
      LogMessage("SendInvite: Complete!");
      if (future_result.Error() != 0) {
        LogMessage("SendInvite: Error %d: %s", future_result.Error(),
                   future_result.ErrorMessage());
      } else {
        auto result = *future_result.Result();
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
  LogMessage("Sample finished.");

  while (!ProcessEvents(1000)) {
  }

  delete sender;
  sender = nullptr;
  delete receiver;
  receiver = nullptr;
  delete app;
  app = nullptr;

  return 0;
}
