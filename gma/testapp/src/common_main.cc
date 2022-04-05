// Copyright 2022 Google Inc. All rights reserved.
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

#include "firebase/gma.h"
#include "firebase/gma/ad_view.h"
#include "firebase/gma/interstitial_ad.h"
#include "firebase/gma/rewarded_ad.h"
#include "firebase/gma/types.h"
#include "firebase/app.h"
#include "firebase/future.h"

// Thin OS abstraction layer.
#include "main.h"  // NOLINT

// A simple listener that logs changes to an AdView.
class LoggingAdViewListener : public firebase::gma::AdListener {
 public:
  LoggingAdViewListener() {}

  void OnAdClicked() override {
    ::LogMessage("AdView ad clicked.");
  }

  void OnAdClosed() override {
    ::LogMessage("AdView ad closed.");
  }

  void OnAdImpression() override {
    ::LogMessage("AdView ad impression.");
  }

  void OnAdOpened() override {
    ::LogMessage("AdView ad opened.");
  }
};

// A simple listener that logs changes to an AdView's bounding box.
class LoggingAdViewBoundedBoxListener
    : public firebase::gma::AdViewBoundingBoxListener {
 public:
  void OnBoundingBoxChanged(firebase::gma::AdView* ad_view,
                            firebase::gma::BoundingBox box) override {
    ::LogMessage("AdView bounding box update x: %d  y: %d  "
      "width: %d  height: %d", box.x, box.y, box.width, box.height);
  }
};

// A simple listener track FullScreen content changes.
class LoggingFullScreenContentListener
    : public firebase::gma::FullScreenContentListener {
 public:
  LoggingFullScreenContentListener() : num_ad_dismissed_(0) { }

  void OnAdClicked() override {
    ::LogMessage("FullScreenContent ad clicked.");
  }

  void OnAdDismissedFullScreenContent() override {
    ::LogMessage("FullScreenContent ad dismissed.");
    num_ad_dismissed_++;
  }

  void OnAdFailedToShowFullScreenContent(
      const firebase::gma::AdError& ad_error) override {
    ::LogMessage("FullScreenContent ad failed to show full screen content,"
      " AdErrorCode: %d", ad_error.code());
  }

  void OnAdImpression() override {
    ::LogMessage("FullScreenContent ad impression.");
  }

  void OnAdShowedFullScreenContent() override {
    ::LogMessage("FullScreenContent ad showed content.");
  }

  uint32_t num_ad_dismissed() const { return num_ad_dismissed_; }

  private:
  uint32_t num_ad_dismissed_;
};

// A simple listener track UserEarnedReward events.
class LoggingUserEarnedRewardListener
    : public firebase::gma::UserEarnedRewardListener {
 public:
  LoggingUserEarnedRewardListener() { }

  void OnUserEarnedReward(const firebase::gma::AdReward& reward) override {
    ::LogMessage("User earned reward amount: %d  type: %s",
      reward.amount(), reward.type().c_str());
  }
};

// A simple listener track ad pay events.
class LoggingPaidEventListener : public firebase::gma::PaidEventListener {
 public:
  LoggingPaidEventListener() { }

  void OnPaidEvent(const firebase::gma::AdValue& value) override {
    ::LogMessage("PaidEvent value: %lld currency_code: %s",
      value.value_micros(), value.currency_code().c_str());
   }
};

void LoadAndShowAdView(const firebase::gma::AdRequest& ad_request);
void LoadAndShowInterstitialAd(const firebase::gma::AdRequest& ad_request);
void LoadAndShowRewardedAd(const firebase::gma::AdRequest& ad_request);

// These ad units IDs have been created specifically for testing, and will
// always return test ads.
#if defined(__ANDROID__)
const char* kBannerAdUnit = "ca-app-pub-3940256099942544/6300978111";
const char* kInterstitialAdUnit = "ca-app-pub-3940256099942544/1033173712";
const char* kRewardedAdUnit = "ca-app-pub-3940256099942544/5224354917";
#else
const char* kBannerAdUnit = "ca-app-pub-3940256099942544/2934735716";
const char* kInterstitialAdUnit = "ca-app-pub-3940256099942544/4411468910";
const char* kRewardedAdUnit = "ca-app-pub-3940256099942544/1712485313";
#endif

// Sample keywords to use in making the request.
static const std::vector<std::string> kKeywords({"GMA", "C++", "Fun"});

// Sample test device IDs to use in making the request.  Add your own here.
const std::vector<std::string> kTestDeviceIDs = {
    "2077ef9a63d2b398840261c8221a0c9b", "098fe087d987c9a878965454a65654d7"};

#if defined(ANDROID)
static const char* kAdNetworkExtrasClassName =
    "com/google/ads/mediation/admob/AdMobAdapter";
#else
static const char* kAdNetworkExtrasClassName = "GADExtras";
#endif

// Function to wait for the completion of a future, and log the error
// if one is encountered.
static void WaitForFutureCompletion(firebase::FutureBase future) {
  while (!ProcessEvents(1000)) {
    if (future.status() != firebase::kFutureStatusPending) {
      break;
    }
  }

  if (future.error() != firebase::gma::kAdErrorCodeNone) {
    LogMessage("ERROR: Action failed with error code %d and message \"%s\".",
               future.error(), future.error_message());
  }
}

// Inittialize GMA, load a Banner, Interstitial and Rewarded Ad.
extern "C" int common_main(int argc, const char* argv[]) {
  firebase::App* app;
  LogMessage("Initializing Firebase App.");

#if defined(__ANDROID__)
  app = ::firebase::App::Create(GetJniEnv(), GetActivity());
#else
  app = ::firebase::App::Create();
#endif  // defined(__ANDROID__)

  LogMessage("Created the Firebase App %x.",
             static_cast<int>(reinterpret_cast<intptr_t>(app)));

  LogMessage("Initializing the GMA with Firebase API.");
  firebase::gma::Initialize(*app);

  WaitForFutureCompletion(firebase::gma::InitializeLastResult());
  if(firebase::gma::InitializeLastResult().error() != firebase::gma::kAdErrorCodeNone) {
    // Initialization Failure. The error was already logged in WaitForFutureCompletion,
    // so simply exit here.
    return -1;
  }

  // Log mediation adapter initialization status.
  for (auto adapter_status :
       firebase::gma::GetInitializationStatus().GetAdapterStatusMap()) {
    LogMessage("GMA Mediation Adapter '%s' %s (latency %d ms): %s",
             adapter_status.first.c_str(),
             (adapter_status.second.is_initialized() ? "loaded" : "NOT loaded"),
             adapter_status.second.latency(),
             adapter_status.second.description().c_str());
  }

  // Configure test device ids before loading ads.
  //
  // This example uses ad units that are specially configured to return test ads
  // for every request. When using your own ad unit IDs, however, it's important
  // to register the device IDs associated with any devices that will be used to
  // test the app. This ensures that regardless of the ad unit ID, those
  // devices will always receive test ads in compliance with AdMob policy.
  //
  // Device IDs can be obtained by checking the logcat or the Xcode log while
  // debugging. They appear as a long string of hex characters.
  firebase::gma::RequestConfiguration request_configuration;
  request_configuration.test_device_ids = kTestDeviceIDs;
  firebase::gma::SetRequestConfiguration(request_configuration);
  
  // 
  // Load and Display a Banner Ad using AdView.
  //
  
  // Create an AdRequest.
  firebase::gma::AdRequest ad_request;

  // Configure additional keywords to be used in targeting.
  for (auto keyword_iter = kKeywords.begin(); keyword_iter != kKeywords.end();
       ++keyword_iter) {
    ad_request.add_keyword((*keyword_iter).c_str());
  }

  // "Extra" key value pairs can be added to the request as well. Typically
  // these are used when testing new features.
  ad_request.add_extra(kAdNetworkExtrasClassName, "the_name_of_an_extra",
    "the_value_for_that_extra");

  LoadAndShowAdView(ad_request);
  LoadAndShowInterstitialAd(ad_request);
  LoadAndShowRewardedAd(ad_request);

  LogMessage("\nAll ad operations complete, terminating GMA");

  firebase::gma::Terminate();
  delete app;

  // Wait until the user kills the app.
  while (!ProcessEvents(1000)) { }

  return 0;
}

void LoadAndShowAdView(const firebase::gma::AdRequest& ad_request) {
  LogMessage("\nLoad and show a banner ad in an AdView:");
  LogMessage("===");
  // Initialize an AdView.
  firebase::gma::AdView* ad_view = new firebase::gma::AdView();
  const firebase::gma::AdSize banner_ad_size = firebase::gma::AdSize::kBanner;
  ad_view->Initialize(GetWindowContext(), kBannerAdUnit, banner_ad_size);

  // Block until the ad view completes initialization.
  WaitForFutureCompletion(ad_view->InitializeLastResult());

  // Check for errors.
  if (ad_view->InitializeLastResult().error() !=
        firebase::gma::kAdErrorCodeNone) {
    LogMessage("AdView initalization failed, error code: %d",
      ad_view->InitializeLastResult().error());
    delete ad_view;
    ad_view = nullptr;
    return;
  }

  // Setup the AdView's listeners.
  LoggingAdViewListener ad_view_listener;
  ad_view->SetAdListener(&ad_view_listener);
  LoggingPaidEventListener paid_event_listener;
  ad_view->SetPaidEventListener(&paid_event_listener);
  LoggingAdViewBoundedBoxListener bounding_box_listener;
  ad_view->SetBoundingBoxListener(&bounding_box_listener);
  
  // Load an ad.
  ad_view->LoadAd(ad_request);
  WaitForFutureCompletion(ad_view->LoadAdLastResult());

  // Check for errors.
  if (ad_view->LoadAdLastResult().error() != firebase::gma::kAdErrorCodeNone) {
    // Log information as to why the loadAd request failed.
    const firebase::gma::AdResult* result_ptr =
      ad_view->LoadAdLastResult().result();
    if (result_ptr != nullptr) {
      LogMessage("AdView::loadAd Failure - Code: %d Message: %s Domain: %s",
        result_ptr->ad_error().code(), result_ptr->ad_error().message().c_str(),
        result_ptr->ad_error().domain().c_str());
    }
    WaitForFutureCompletion(ad_view->Destroy());
    delete ad_view;
    ad_view = nullptr;
    return;
  }

  // Log the loaded ad's dimensions.
  const firebase::gma::AdSize ad_size = ad_view->ad_size();
  LogMessage("AdView loaded ad width: %d height: %d", ad_size.width(),
    ad_size.height());

  // Show the ad.
  LogMessage("Showing the banner ad.");
  WaitForFutureCompletion(ad_view->Show());

  // Move to each of the six pre-defined positions.
  LogMessage("Moving the banner ad to top-center.");
  ad_view->SetPosition(firebase::gma::AdView::kPositionTop);
  WaitForFutureCompletion(ad_view->SetPositionLastResult());

  LogMessage("Moving the banner ad to top-left.");
  ad_view->SetPosition(firebase::gma::AdView::kPositionTopLeft);
  WaitForFutureCompletion(ad_view->SetPositionLastResult());

  LogMessage("Moving the banner ad to top-right.");
  ad_view->SetPosition(firebase::gma::AdView::kPositionTopRight);
  WaitForFutureCompletion(ad_view->SetPositionLastResult());

  LogMessage("Moving the banner ad to bottom-center.");
  ad_view->SetPosition(firebase::gma::AdView::kPositionBottom);
  WaitForFutureCompletion(ad_view->SetPositionLastResult());

  LogMessage("Moving the banner ad to bottom-left.");
  ad_view->SetPosition(firebase::gma::AdView::kPositionBottomLeft);
  WaitForFutureCompletion(ad_view->SetPositionLastResult());

  LogMessage("Moving the banner ad to bottom-right.");
  ad_view->SetPosition(firebase::gma::AdView::kPositionBottomRight);
  WaitForFutureCompletion(ad_view->SetPositionLastResult());

  // Try some coordinate moves.
  LogMessage("Moving the banner ad to (100, 300).");
  ad_view->SetPosition(100, 300);
  WaitForFutureCompletion(ad_view->SetPositionLastResult());

  LogMessage("Moving the banner ad to (100, 400).");
  ad_view->SetPosition(100, 400);
  WaitForFutureCompletion(ad_view->SetPositionLastResult());

  // Try hiding and showing the BannerView.
  LogMessage("Hiding the banner ad.");
  ad_view->Hide();
  WaitForFutureCompletion(ad_view->HideLastResult());

  LogMessage("Showing the banner ad.");
  ad_view->Show();
  WaitForFutureCompletion(ad_view->ShowLastResult());

  LogMessage("Hiding the banner ad again now that we're done with it.");
  ad_view->Hide();
  WaitForFutureCompletion(ad_view->HideLastResult());

  // Clean up the ad view.
  ad_view->Destroy();
  WaitForFutureCompletion(ad_view->DestroyLastResult());
  delete ad_view;
  ad_view = nullptr;
}

void LoadAndShowInterstitialAd(const firebase::gma::AdRequest& ad_request) {
  LogMessage("\nLoad and show an interstitial ad:");
  LogMessage("===");
  // Initialize an InterstitialAd.
  firebase::gma::InterstitialAd* interstitial_ad = new firebase::gma::InterstitialAd();
  interstitial_ad->Initialize(GetWindowContext());

  // Block until the interstitial ad completes initialization.
  WaitForFutureCompletion(interstitial_ad->InitializeLastResult());

  // Check for errors.
  if (interstitial_ad->InitializeLastResult().error() != firebase::gma::kAdErrorCodeNone) {
    delete interstitial_ad;
    interstitial_ad = nullptr;
    return;
  }

  // Setup the interstitial ad's listeners.
  LoggingFullScreenContentListener fullscreen_content_listener;
  interstitial_ad->SetFullScreenContentListener(&fullscreen_content_listener);
  LoggingPaidEventListener paid_event_listener;
  interstitial_ad->SetPaidEventListener(&paid_event_listener);

  // Load an ad.
  interstitial_ad->LoadAd(kInterstitialAdUnit, ad_request);
  WaitForFutureCompletion(interstitial_ad->LoadAdLastResult());

  // Check for errors.
  if (interstitial_ad->LoadAdLastResult().error() != firebase::gma::kAdErrorCodeNone) {
    // Log information as to why the loadAd request failed.
    const firebase::gma::AdResult* result_ptr =
      interstitial_ad->LoadAdLastResult().result();
    if (result_ptr != nullptr) {
      LogMessage("InterstitialAd::loadAd Failure - Code: %d Message: %s Domain: %s",
        result_ptr->ad_error().code(), result_ptr->ad_error().message().c_str(),
        result_ptr->ad_error().domain().c_str());
    }
    delete interstitial_ad;
    interstitial_ad = nullptr;
    return;
  }

  // Show the ad.
  LogMessage("Showing the interstitial ad.");
  interstitial_ad->Show();
  WaitForFutureCompletion(interstitial_ad->ShowLastResult());

  // Wait for the user to close the interstitial.
  while (fullscreen_content_listener.num_ad_dismissed() == 0) {
    ProcessEvents(1000);
  }

  // Clean up the interstitial ad.
  delete interstitial_ad;
  interstitial_ad = nullptr;
}

// WIP
void LoadAndShowRewardedAd(const firebase::gma::AdRequest& ad_request) {
  LogMessage("\nLoad and show a rewarded ad:");
  LogMessage("===");
  // Initialize a RewardedAd.
  firebase::gma::RewardedAd* rewarded_ad = new firebase::gma::RewardedAd();
  rewarded_ad->Initialize(GetWindowContext());

  // Block until the interstitial ad completes initialization.
  WaitForFutureCompletion(rewarded_ad->InitializeLastResult());

  // Check for errors.
  if (rewarded_ad->InitializeLastResult().error() != firebase::gma::kAdErrorCodeNone) {
    delete rewarded_ad;
    rewarded_ad = nullptr;
    return;
  }

  // Setup the rewarded ad's lifecycle listeners.
  LoggingFullScreenContentListener fullscreen_content_listener;
  rewarded_ad->SetFullScreenContentListener(&fullscreen_content_listener);
  LoggingPaidEventListener paid_event_listener;
  rewarded_ad->SetPaidEventListener(&paid_event_listener);

  // Load an ad.
  rewarded_ad->LoadAd(kRewardedAdUnit, ad_request);
  WaitForFutureCompletion(rewarded_ad->LoadAdLastResult());

  // Check for errors.
  if (rewarded_ad->LoadAdLastResult().error() != firebase::gma::kAdErrorCodeNone) {
    // Log information as to why the loadAd request failed.
    const firebase::gma::AdResult* result_ptr =
      rewarded_ad->LoadAdLastResult().result();
    if (result_ptr != nullptr) {
      LogMessage("RewardedAd::loadAd Failure - Code: %d Message: %s Domain: %s",
        result_ptr->ad_error().code(), result_ptr->ad_error().message().c_str(),
        result_ptr->ad_error().domain().c_str());
    }
    delete rewarded_ad;
    rewarded_ad  = nullptr;
    return;
  }

  // Show the ad.
  LogMessage("Showing the rewarded ad.");
  LoggingUserEarnedRewardListener user_earned_reward_listener;
  rewarded_ad->Show(&user_earned_reward_listener);
  WaitForFutureCompletion(rewarded_ad->ShowLastResult());

  // Wait for the user to close the interstitial.
  while (fullscreen_content_listener.num_ad_dismissed() == 0) {
    ProcessEvents(1000);
  }

  // Clean up the interstitial ad.
  delete rewarded_ad;
  rewarded_ad = nullptr;
}
