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
#include "firebase/admob/rewarded_video.h"
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
      firebase::admob::BannerView::PresentationState state) override {
    ::LogMessage("BannerView PresentationState has changed to %d.", state);
  }
  void OnBoundingBoxChanged(firebase::admob::BannerView* banner_view,
                            firebase::admob::BoundingBox box) override {
    ::LogMessage(
        "BannerView BoundingBox has changed to (x: %d, y: %d, width: %d, "
        "height %d).",
        box.x, box.y, box.width, box.height);
  }
};

// A simple listener that logs changes to an InterstitialAd.
class LoggingInterstitialAdListener
    : public firebase::admob::InterstitialAd::Listener {
 public:
  LoggingInterstitialAdListener() {}
  void OnPresentationStateChanged(
      firebase::admob::InterstitialAd* interstitial_ad,
      firebase::admob::InterstitialAd::PresentationState state) override {
    ::LogMessage("InterstitialAd PresentationState has changed to %d.", state);
  }
};

// A simple listener that logs changes to rewarded video state.
class LoggingRewardedVideoListener
    : public firebase::admob::rewarded_video::Listener {
 public:
  LoggingRewardedVideoListener() {}
  void OnRewarded(firebase::admob::rewarded_video::RewardItem reward) override {
    ::LogMessage("Rewarding user with %f %s.", reward.amount,
                 reward.reward_type.c_str());
  }
  void OnPresentationStateChanged(
      firebase::admob::rewarded_video::PresentationState state) override {
    ::LogMessage("Rewarded video PresentationState has changed to %d.", state);
  }
};

// The AdMob app IDs for the test app.
#if defined(__ANDROID__)
// If you change the AdMob app ID for your Android app, make sure to change it
// in AndroidManifest.xml as well.
const char* kAdMobAppID = "YOUR_ANDROID_ADMOB_APP_ID";
#else
// If you change the AdMob app ID for your iOS app, make sure to change the
// value for "GADApplicationIdentifier" in your Info.plist as well.
const char* kAdMobAppID = "YOUR_IOS_ADMOB_APP_ID";
#endif

// These ad units IDs have been created specifically for testing, and will
// always return test ads.
#if defined(__ANDROID__)
const char* kBannerAdUnit = "ca-app-pub-3940256099942544/6300978111";
const char* kInterstitialAdUnit = "ca-app-pub-3940256099942544/1033173712";
const char* kRewardedVideoAdUnit = "ca-app-pub-3940256099942544/5224354917";
#else
const char* kBannerAdUnit = "ca-app-pub-3940256099942544/2934735716";
const char* kInterstitialAdUnit = "ca-app-pub-3940256099942544/4411468910";
const char* kRewardedVideoAdUnit = "ca-app-pub-3940256099942544/1712485313";
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
    if (future.status() != firebase::kFutureStatusPending) {
      break;
    }
  }

  if (future.error() != firebase::admob::kAdMobErrorNone) {
    LogMessage("ERROR: Action failed with error code %d and message \"%s\".",
               future.error(), future.error_message());
  }
}

// Execute all methods of the C++ admob API.
extern "C" int common_main(int argc, const char* argv[]) {
  firebase::App* app;
  LogMessage("Initializing the AdMob library.");

#if defined(__ANDROID__)
  app = ::firebase::App::Create(GetJniEnv(), GetActivity());
#else
  app = ::firebase::App::Create();
#endif  // defined(__ANDROID__)

  LogMessage("Created the Firebase App %x.",
             static_cast<int>(reinterpret_cast<intptr_t>(app)));

  LogMessage("Initializing the AdMob with Firebase API.");
  firebase::admob::Initialize(*app, kAdMobAppID);

  firebase::admob::AdRequest request;
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
  static const firebase::admob::KeyValuePair kRequestExtras[] = {
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
      sizeof(kTestDeviceIDs) / sizeof(kTestDeviceIDs[0]);
  request.test_device_ids = kTestDeviceIDs;

  // Create an ad size for the BannerView.
  firebase::admob::AdSize banner_ad_size;
  banner_ad_size.ad_size_type = firebase::admob::kAdSizeStandard;
  banner_ad_size.width = kBannerWidth;
  banner_ad_size.height = kBannerHeight;

  LogMessage("Creating the BannerView.");
  firebase::admob::BannerView* banner = new firebase::admob::BannerView();
  banner->Initialize(GetWindowContext(), kBannerAdUnit, banner_ad_size);

  WaitForFutureCompletion(banner->InitializeLastResult());

  // Set the listener.
  LoggingBannerViewListener banner_listener;
  banner->SetListener(&banner_listener);

  // Load the banner ad.
  LogMessage("Loading a banner ad.");
  banner->LoadAd(request);

  WaitForFutureCompletion(banner->LoadAdLastResult());

  // Make the BannerView visible.
  LogMessage("Showing the banner ad.");
  banner->Show();

  WaitForFutureCompletion(banner->ShowLastResult());

  // Move to each of the six pre-defined positions.
  LogMessage("Moving the banner ad to top-center.");
  banner->MoveTo(firebase::admob::BannerView::kPositionTop);

  WaitForFutureCompletion(banner->MoveToLastResult());

  LogMessage("Moving the banner ad to top-left.");
  banner->MoveTo(firebase::admob::BannerView::kPositionTopLeft);

  WaitForFutureCompletion(banner->MoveToLastResult());

  LogMessage("Moving the banner ad to top-right.");
  banner->MoveTo(firebase::admob::BannerView::kPositionTopRight);

  WaitForFutureCompletion(banner->MoveToLastResult());

  LogMessage("Moving the banner ad to bottom-center.");
  banner->MoveTo(firebase::admob::BannerView::kPositionBottom);

  WaitForFutureCompletion(banner->MoveToLastResult());

  LogMessage("Moving the banner ad to bottom-left.");
  banner->MoveTo(firebase::admob::BannerView::kPositionBottomLeft);

  WaitForFutureCompletion(banner->MoveToLastResult());

  LogMessage("Moving the banner ad to bottom-right.");
  banner->MoveTo(firebase::admob::BannerView::kPositionBottomRight);

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

  LogMessage("Hiding the banner ad now that we're done with it.");
  banner->Hide();

  WaitForFutureCompletion(banner->HideLastResult());

  // Create and test InterstitialAd.
  LogMessage("Creating the InterstitialAd.");
  firebase::admob::InterstitialAd* interstitial =
      new firebase::admob::InterstitialAd();
  interstitial->Initialize(GetWindowContext(), kInterstitialAdUnit);

  WaitForFutureCompletion(interstitial->InitializeLastResult());

  // Set the listener.
  LoggingInterstitialAdListener interstitial_listener;
  interstitial->SetListener(&interstitial_listener);

  // When the InterstitialAd is initialized, load an ad.
  LogMessage("Loading an interstitial ad.");
  interstitial->LoadAd(request);

  WaitForFutureCompletion(interstitial->LoadAdLastResult());

  // When the InterstitialAd has loaded an ad, show it.
  LogMessage("Showing the interstitial ad.");
  interstitial->Show();

  WaitForFutureCompletion(interstitial->ShowLastResult());

  // Wait for the user to close the interstitial.
  while (interstitial->presentation_state() !=
         firebase::admob::InterstitialAd::PresentationState::
             kPresentationStateHidden) {
    ProcessEvents(1000);
  }

  // Start up rewarded video ads and associated mediation adapters.
  LogMessage("Initializing rewarded video.");
  namespace rewarded_video = firebase::admob::rewarded_video;
  rewarded_video::Initialize();

  WaitForFutureCompletion(rewarded_video::InitializeLastResult());

  LogMessage("Setting rewarded video listener.");
  LoggingRewardedVideoListener rewarded_listener;
  rewarded_video::SetListener(&rewarded_listener);

  LogMessage("Loading a rewarded video ad.");
  rewarded_video::LoadAd(kRewardedVideoAdUnit, request);

  WaitForFutureCompletion(rewarded_video::LoadAdLastResult());

  // If an ad has loaded, show it. If the user watches all the way through, the
  // LoggingRewardedVideoListener will log a reward!
  if (rewarded_video::LoadAdLastResult().error() ==
      firebase::admob::kAdMobErrorNone) {
    LogMessage("Showing a rewarded video ad.");
    rewarded_video::Show(GetWindowContext());

    WaitForFutureCompletion(rewarded_video::ShowLastResult());

    // Normally Pause and Resume would be called in response to the app pausing
    // or losing focus. This is just a test.
    LogMessage("Pausing.");
    rewarded_video::Pause();

    WaitForFutureCompletion(rewarded_video::PauseLastResult());

    LogMessage("Resuming.");
    rewarded_video::Resume();

    WaitForFutureCompletion(rewarded_video::ResumeLastResult());
  }

  LogMessage("Done!");

  // Wait until the user kills the app.
  while (!ProcessEvents(1000)) {
  }

  delete banner;
  delete interstitial;
  rewarded_video::Destroy();
  firebase::admob::Terminate();
  delete app;

  return 0;
}
