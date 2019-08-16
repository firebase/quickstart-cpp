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

#include <assert.h>
#include <string.h>

#include "firebase/app.h"
#include "firebase/dynamic_links.h"
#include "firebase/dynamic_links/components.h"
#include "firebase/future.h"
#include "firebase/util.h"
// Thin OS abstraction layer.
#include "main.h"  // NOLINT

// Invalid domain, used to make sure the user sets a valid domain.
#define INVALID_DOMAIN_URI_PREFIX "THIS_IS_AN_INVALID_DOMAIN"

static const char* kDomainUriPrefixInvalidError =
    "kDomainUriPrefix is not valid, link shortening will fail.\n"
    "To resolve this:\n"
    "* Goto the Firebase console https://firebase.google.com/console/\n"
    "* Click on the Dynamic Links tab\n"
    "* Copy the URI prefix e.g https://x20yz.app.goo.gl\n"
    "* Replace the value of kDomainUriPrefix with the copied URI prefix.\n";

// IMPORTANT: You need to set this to a valid URI prefix from the Firebase
// console (see kDomainUriPrefixInvalidError for the details).
static const char* kDomainUriPrefix = INVALID_DOMAIN_URI_PREFIX;

// Displays a received dynamic link.
class Listener : public firebase::dynamic_links::Listener {
 public:
  // Called on the client when a dynamic link arrives.
  void OnDynamicLinkReceived(
      const firebase::dynamic_links::DynamicLink* dynamic_link) override {
    LogMessage("Received link: %s", dynamic_link->url.c_str());
  }
};

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

// Show a generated link.
void ShowGeneratedLink(
    const firebase::dynamic_links::GeneratedDynamicLink& generated_link,
    const char* operation_description) {
  if (!generated_link.warnings.empty()) {
    LogMessage("%s generated warnings:", operation_description);
    for (auto it = generated_link.warnings.begin();
         it != generated_link.warnings.end(); ++it) {
      LogMessage("  %s", it->c_str());
    }
  }
  LogMessage("url: %s", generated_link.url.c_str());
}

// Wait for dynamic link generation to complete, logging the result.
void WaitForAndShowGeneratedLink(
    const firebase::Future<firebase::dynamic_links::GeneratedDynamicLink>&
        generated_dynamic_link_future,
    const char* operation_description) {
  LogMessage("%s...", operation_description);
  WaitForCompletion(generated_dynamic_link_future, operation_description);
  if (generated_dynamic_link_future.error() != 0) {
    LogMessage("ERROR: %s failed with error %d: %s", operation_description,
               generated_dynamic_link_future.error(),
               generated_dynamic_link_future.error_message());
    return;
  }
  ShowGeneratedLink(*generated_dynamic_link_future.result(),
                    operation_description);
}

// Execute all methods of the C++ Dynamic Links API.
extern "C" int common_main(int argc, const char* argv[]) {
  namespace dynamic_links = ::firebase::dynamic_links;
  ::firebase::App* app;
  Listener* link_listener = new Listener;

  LogMessage("Initialize the Firebase Dynamic Links library");
#if defined(__ANDROID__)
  app = ::firebase::App::Create(GetJniEnv(), GetActivity());
#else
  app = ::firebase::App::Create();
#endif  // defined(__ANDROID__)

  LogMessage("Created the Firebase app %x",
             static_cast<int>(reinterpret_cast<intptr_t>(app)));

  ::firebase::ModuleInitializer initializer;
  initializer.Initialize(app, link_listener,
                         [](::firebase::App* app, void* listener) {
                           LogMessage("Try to initialize Dynamic Links");
                           return ::firebase::dynamic_links::Initialize(
                               *app, reinterpret_cast<Listener*>(listener));
                         });
  while (initializer.InitializeLastResult().status() !=
         firebase::kFutureStatusComplete) {
    if (ProcessEvents(100)) return 1;  // exit if requested
  }
  if (initializer.InitializeLastResult().error() != 0) {
    LogMessage("Failed to initialize Firebase Dynamic Links: %s",
               initializer.InitializeLastResult().error_message());
    ProcessEvents(2000);
    return 1;
  }

  LogMessage("Initialized the Firebase Dynamic Links API");

  firebase::dynamic_links::GoogleAnalyticsParameters analytics_parameters;
  analytics_parameters.source = "mysource";
  analytics_parameters.medium = "mymedium";
  analytics_parameters.campaign = "mycampaign";
  analytics_parameters.term = "myterm";
  analytics_parameters.content = "mycontent";

  firebase::dynamic_links::IOSParameters ios_parameters("com.myapp.bundleid");
  ios_parameters.fallback_url = "https://mysite/fallback";
  ios_parameters.custom_scheme = "mycustomscheme";
  ios_parameters.minimum_version = "1.2.3";
  ios_parameters.ipad_bundle_id = "com.myapp.bundleid.ipad";
  ios_parameters.ipad_fallback_url = "https://mysite/fallbackipad";

  firebase::dynamic_links::ITunesConnectAnalyticsParameters
      app_store_parameters;
  app_store_parameters.affiliate_token = "abcdefg";
  app_store_parameters.campaign_token = "hijklmno";
  app_store_parameters.provider_token = "pq-rstuv";

  firebase::dynamic_links::AndroidParameters android_parameters(
      "com.myapp.packageid");
  android_parameters.fallback_url = "https://mysite/fallback";
  android_parameters.minimum_version = 12;

  firebase::dynamic_links::SocialMetaTagParameters social_parameters;
  social_parameters.title = "My App!";
  social_parameters.description = "My app is awesome!";
  social_parameters.image_url = "https://mysite.com/someimage.jpg";

  firebase::dynamic_links::DynamicLinkComponents components(
      "https://google.com/abc", kDomainUriPrefix);
  components.google_analytics_parameters = &analytics_parameters;
  components.ios_parameters = &ios_parameters;
  components.itunes_connect_analytics_parameters = &app_store_parameters;
  components.android_parameters = &android_parameters;
  components.social_meta_tag_parameters = &social_parameters;

  dynamic_links::GeneratedDynamicLink long_link;
  {
    const char* description = "Generate long link from components";
    long_link = dynamic_links::GetLongLink(components);
    LogMessage("%s...", description);
    ShowGeneratedLink(long_link, description);
  }

  if (strcmp(kDomainUriPrefix, INVALID_DOMAIN_URI_PREFIX) == 0) {
    LogMessage(kDomainUriPrefixInvalidError);
  } else {
    {
      firebase::Future<dynamic_links::GeneratedDynamicLink> link_future =
          dynamic_links::GetShortLink(components);
      WaitForAndShowGeneratedLink(link_future,
                                  "Generate short link from components");
    }
    if (!long_link.url.empty()) {
      dynamic_links::DynamicLinkOptions options;
      options.path_length = firebase::dynamic_links::kPathLengthShort;
      firebase::Future<dynamic_links::GeneratedDynamicLink> link_future =
          dynamic_links::GetShortLink(long_link.url.c_str(), options);
      WaitForAndShowGeneratedLink(link_future, "Generate short from long link");
    }
  }

  // Wait until the user wants to quit the app.
  while (!ProcessEvents(1000)) {
  }

  dynamic_links::Terminate();
  delete link_listener;
  delete app;

  return 0;
}
