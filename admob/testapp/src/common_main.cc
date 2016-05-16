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

#include "firebase/admob.h"
#include "firebase/admob/banner_view.h"
#include "firebase/admob/interstitial_ad.h"
#include "firebase/admob/types.h"
#include "firebase/app.h"
#include "firebase/future.h"

// Thin OS abstraction layer.
#include "main.h"  // NOLINT

// A simple listener that logs changes to a BannerView.
class LoggingBannerViewListener : public firebase::admob::BannerView::Listener {
 public:
  LoggingBannerViewListener() {}
  void OnPresentationStateChanged(
      firebase::admob::BannerView* banner_view,
      firebase::admob::BannerView::PresentationState new_state) {
    ::LogMessage("BannerView PresentationState has changed to %d.", new_state);
  }

  void OnBoundingBoxChanged(firebase::admob::BannerView* banner_view,
                            firebase::admob::BoundingBox new_box) {
    ::LogMessage(
        "BannerView BoundingBox has changed to (x: %d, y: %d, width: %d, "
        "height %d).",
        new_box.x, new_box.y, new_box.width, new_box.height);
  }
};

// A simple listener that logs changes to an InterstitialAd.
class LoggingInterstitialAdListener
    : public firebase::admob::InterstitialAd::Listener {
 public:
  LoggingInterstitialAdListener() {}
  void OnPresentationStateChanged(
      firebase::admob::InterstitialAd* interstitial_ad,
      firebase::admob::InterstitialAd::PresentationState new_state) {
    ::LogMessage("InterstitialAd PresentationState has changed to %d.",
                 new_state);
  }
};

// These ad units are configured to always serve test ads.
#if defined(__ANDROID__)
const char* kBannerAdUnit = "ca-app-pub-3940256099942544/6300978111";
const char* kInterstitialAdUnit = "ca-app-pub-3940256099942544/1033173712";
#else
const char* kBannerAdUnit = "ca-app-pub-3940256099942544/2934735716";
const char* kInterstitialAdUnit = "ca-app-pub-3940256099942544/4411468910";
#endif

// Standard mobile banner size is 320x50.
static const int kBannerWidth = 320;
static const int kBannerHeight = 50;

// Sample keywords to use in making the request.
static const char* kKeywords[] = {"AdMob", "C++", "Fun"};

// Sample test device IDs to use in making the request.
static const char* kTestDeviceIDs[] = {"2077ef9a63d2b398840261c8221a0c9b",
                                       "098fe087d987c9a878965454a65654d7"};

// Sample birthday value to use in making the request.
static const int kBirthdayDay = 10;
static const int kBirthdayMonth = 11;
static const int kBirthdayYear = 1976;

static void WaitForFutureCompletion(firebase::FutureBase future) {
  while (!ProcessEvents(1000)) {
    if (future.Status() != ::firebase::kFutureStatusPending) {
      break;
    }
  }

  if (future.Error() != ::firebase::admob::kAdMobErrorNone) {
    LogMessage("Action failed with error code %d and message \"%s\".",
               future.Error(), future.ErrorMessage());
  }
}

// Execute all methods of the C++ admob API.
extern "C" int common_main(int argc, const char* argv[]) {
  namespace admob = ::firebase::admob;
  ::firebase::App* app;
  LogMessage("Initializing the AdMob library.");
#if defined(__ANDROID__)
  app = ::firebase::App::Create(::firebase::AppOptions(), GetJniEnv(),
                                GetActivity());
#else
  app = ::firebase::App::Create(::firebase::AppOptions());
#endif  // defined(__ANDROID__)

  LogMessage("Created the Firebase App %x.",
             static_cast<int>(reinterpret_cast<intptr_t>(app)));

  LogMessage("Initializing the AdMob with Firebase API.");
  admob::Initialize(*app);

  ::firebase::admob::AdRequest request;
  // If the app is aware of the user's gender, it can be added to the targeting
  // information. Otherwise, "unknown" should be used.
  request.gender = firebase::admob::kGenderUnknown;

  // This value allows publishers to specify whether they would like the request
  // to be treated as child-directed for purposes of the Children’s Online
  // Privacy Protection Act (COPPA).
  // See http://business.ftc.gov/privacy-and-security/childrens-privacy.
  request.tagged_for_child_directed_treatment =
      firebase::admob::kChildDirectedTreatmentStateTagged;

  // The user's birthday, if known. Note that months are indexed from one.
  request.birthday_day = kBirthdayDay;
  request.birthday_month = kBirthdayMonth;
  request.birthday_year = kBirthdayYear;

  // Additional keywords to be used in targeting.
  request.keyword_count = sizeof(kKeywords) / sizeof(kKeywords[0]);
  request.keywords = kKeywords;

  // "Extra" key value pairs can be added to the request as well. Typically
  // these are used when testing new features.
  static const ::firebase::admob::KeyValuePair kRequestExtras[] = {
      {"the_name_of_an_extra", "the_value_for_that_extra"}};
  request.extras_count = sizeof(kRequestExtras) / sizeof(kRequestExtras[0]);
  request.extras = kRequestExtras;

  // This example uses ad units that are specially configured to return test ads
  // for every request. When using your own ad unit IDs, however, it's important
  // to register the device IDs associated with any devices that will be used to
  // test the app. This ensures that regardless of the ad unit ID, those
  // devices will always receive test ads in compliance with AdMob policy.
  //
  // Device IDs can be obtained by checking the logcat or the Xcode log while
  // debugging. They appear as a long string of hex characters.
  request.test_device_id_count =
      sizeof(kTestDeviceIDs) / sizeof(kTestDeviceIDs);
  request.test_device_ids = kTestDeviceIDs;

  // Create an ad size for the BannerView.
  admob::AdSize ad_size;
  ad_size.ad_size_type = admob::kAdSizeStandard;
  ad_size.width = kBannerWidth;
  ad_size.height = kBannerHeight;

  LogMessage("Creating the BannerView.");
  LoggingBannerViewListener banner_listener;
  ::firebase::admob::BannerView* banner = new admob::BannerView();
  banner->SetListener(&banner_listener);
  banner->Initialize(GetWindowContext(), kBannerAdUnit, ad_size);

  WaitForFutureCompletion(banner->InitializeLastResult());

  // Make the BannerView visible.
  LogMessage("Showing the banner ad.");
  banner->Show();

  WaitForFutureCompletion(banner->ShowLastResult());

  // When the BannerView is visible, load an ad into it.
  LogMessage("Loading a banner ad.");
  banner->LoadAd(request);

  WaitForFutureCompletion(banner->LoadAdLastResult());

  // Move to each of the six pre-defined positions.
  LogMessage("Moving the banner ad to top-center.");
  banner->MoveTo(admob::BannerView::kPositionTop);

  WaitForFutureCompletion(banner->MoveToLastResult());

  LogMessage("Moving the banner ad to top-left.");
  banner->MoveTo(admob::BannerView::kPositionTopLeft);

  WaitForFutureCompletion(banner->MoveToLastResult());

  LogMessage("Moving the banner ad to top-right.");
  banner->MoveTo(admob::BannerView::kPositionTopRight);

  WaitForFutureCompletion(banner->MoveToLastResult());

  LogMessage("Moving the banner ad to bottom-center.");
  banner->MoveTo(admob::BannerView::kPositionBottom);

  WaitForFutureCompletion(banner->MoveToLastResult());

  LogMessage("Moving the banner ad to bottom-left.");
  banner->MoveTo(admob::BannerView::kPositionBottomLeft);

  WaitForFutureCompletion(banner->MoveToLastResult());

  LogMessage("Moving the banner ad to bottom-right.");
  banner->MoveTo(admob::BannerView::kPositionBottomRight);

  WaitForFutureCompletion(banner->MoveToLastResult());

  // Try some coordinate moves.
  LogMessage("Moving the banner ad to (100, 300).");
  banner->MoveTo(100, 300);

  WaitForFutureCompletion(banner->MoveToLastResult());

  LogMessage("Moving the banner ad to (100, 400).");
  banner->MoveTo(100, 400);

  WaitForFutureCompletion(banner->MoveToLastResult());

  // Try hiding and showing the BannerView.
  LogMessage("Hiding the banner ad.");
  banner->Hide();

  WaitForFutureCompletion(banner->HideLastResult());

  LogMessage("Showing the banner ad.");
  banner->Show();

  WaitForFutureCompletion(banner->ShowLastResult());

  // A few last moves after showing it again.
  LogMessage("Moving the banner ad to (100, 300).");
  banner->MoveTo(100, 300);

  WaitForFutureCompletion(banner->MoveToLastResult());

  LogMessage("Moving the banner ad to (100, 400).");
  banner->MoveTo(100, 400);

  WaitForFutureCompletion(banner->MoveToLastResult());

  // Create and test InterstitialAd.
  LogMessage("Creating the InterstitialAd.");
  LoggingInterstitialAdListener interstitial_listener;
  ::firebase::admob::InterstitialAd* interstitial = new admob::InterstitialAd();
  interstitial->SetListener(&interstitial_listener);
  interstitial->Initialize(GetWindowContext(), kInterstitialAdUnit);

  WaitForFutureCompletion(interstitial->InitializeLastResult());

  // When the InterstitialAd is initialized, load an ad.
  LogMessage("Loading an interstitial ad.");
  interstitial->LoadAd(request);

  WaitForFutureCompletion(interstitial->LoadAdLastResult());

  // When the InterstitialAd has loaded an ad, show it.
  LogMessage("Showing the interstitial ad.");
  interstitial->Show();

  // Wait until the user kills the app.
  while (!ProcessEvents(1000)) {
  }

  delete interstitial;
  delete banner;
  admob::Terminate();
  delete app;

  return 0;
}
