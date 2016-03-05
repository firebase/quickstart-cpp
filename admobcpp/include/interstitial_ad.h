// Copyright 2016 Google Inc. All Rights Reserved.

#ifndef FIREBASE_ADMOB_INTERSTITIAL_AD_H
#define FIREBASE_ADMOB_INTERSTITIAL_AD_H_

#include "types.h"

namespace firebase {
namespace admob {

/// The lifecycle states of an InterstitialAd.
enum InterstitialAdLifecycleState {
  /// InterstitialAd is in the process of being initialized.
  kInterstitialAdInitializing = 0,
  /// InterstitialAd is ready to load an ad.
  kInterstitialAdInitialized,
  /// InterstitialAd has made an ad request and is waiting for a response.
  kInterstitialAdLoading,
  /// InterstitialAd has received an ad and is displaying it.
  kInterstitialAdLoaded,
  /// InterstitialAd has shown (or is curently showing) the ad it last loaded.
  kInterstitialAdHasBeenShown,
  /// InterstitialAd tried to load an ad, but failed due to an internal error.
  kInterstitialAdFailedInternalError,
  /// InterstitialAd tried to load an ad, but failed due to an invalid request.
  kInterstitialAdFailedInvalidRequest,
  /// InterstitialAd tried to load an ad, but failed due to a network error.
  kInterstitialAdFailedNetworkError,
  /// InterstitialAd tried to load an ad, but failed due to lack of inventory.
  kInterstitialAdFailedNoFill,
  /// InterstitialAd has completely failed and should be discarded.
  kInterstitialAdFatalError
};

/// The presentation states of an InterstitialAd.
enum InterstitialAdPresentationState {
  /// InterstitialAd is not currently being shown.
  kInterstitialAdHidden = 0,
  /// InterstitialAd is being shown or has caused focus to leave the
  /// application (e.g. when opening an external browser during a clickthrough).
  kInterstitialAdCoveringUI
};

/// Loads and displays interstitial ads.
class InterstitialAd {
 public:

  /// Creates a new InterstitialAd that can be used to load and display
  /// multiple interstitial ads one at a time.
  InterstitialAd(AdParent parent, const char* ad_unit_id);

  /// Destructor.
  ~InterstitialAd();

  /// Begins an asynchronous request for an ad. The
  /// (@see InterstitialAd::GetState) method can be used to track the progress
  /// of the request.
  void LoadAd(AdRequest request);

  /// Shows the interstitial ad, if it has been loaded. This should not be
  /// called unless (@see InterstitialAd::GetState) returns
  /// kInterstitialAdLoaded, indicating that an ad has been loaded and is ready
  /// to be displayed.
  void Show();

  /// Returns the current state of the InterstitialAd within its lifecycle (i.e.
  /// whether it's currently loading an ad, has failed, etc.).
  ///
  /// @return The current lifecycle state.
  InterstitialAdLifecycleState GetLifecycleState();

  /// Returns the current presentation state of the InterstitialAd.
  ///
  /// @return The current presentation state.
  InterstitialAdPresentationState GetPresentationState();

private:
  jobject helper_;
};

} // namespace admob
} // namespace firebase

#endif
