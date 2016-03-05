// Copyright 2016 Google Inc. All Rights Reserved.

#ifndef FIREBASE_ADMOB_AD_VIEW_H_
#define FIREBASE_ADMOB_AD_VIEW_H_

#include "types.h"

namespace firebase {
namespace admob {

/// The lifecycle states of an BannerView.
enum BannerViewLifecycleState {
  /// BannerView is in the process of being initialized.
  kBannerViewInitializing = 0,
  /// BannerView is ready to load its first ad.
  kBannerViewInitialized,
  /// BannerView has sent a request and is waiting for a response.
  kBannerViewLoading,
  /// BannerView has received an ad and is displaying it.
  kBannerViewLoaded,
  /// BannerView tried to load an ad, but failed due to an internal error.
  kBannerViewFailedInternalError,
  /// BannerView tried to load an ad, but failed due to an invalid request.
  kBannerViewFailedInvalidRequest,
  /// BannerView tried to load an ad, but failed due to a network error.
  kBannerViewFailedNetworkError,
  /// BannerView tried to load an ad, but failed due to lack of inventory.
  kBannerViewFailedNoFill,
  /// BannerView has completely failed and should be discarded.
  kBannerViewFatalError
};

enum BannerViewPresentationState {
  /// BannerView is currently hidden.
  kBannerViewHidden = 0,
  /// BannerView is visible, but does not contain an ad.
  kBannerViewVisibleWithoutAd,
  /// BannerView is visible and contains an ad.
  kBannerViewVisibleWithAd,
  /// BannerView is visible and has opened a partial overlay on the screen.
  kBannerViewOpenedPartialOverlay,
  /// BannerView is completely covering the screen or has caused focus to leave
  /// the application (e.g. when opening an external browser during a clickthrough).
  kBannerViewCoveringUI
};

/// The possible screen positions for an AdView.
enum BannerViewPosition {
  /// Top of the screen, horizontally centered.
  kBannerViewPositionTop = 0,
  /// Bottom of the screen, horizontally centered.
  kBannerViewPositionBottom,
  /// Top-left corner screen.
  kBannerViewPositionTopLeft,
  /// Top-right corner screen.
  kBannerViewPositionTopRight,
  /// Bottom-left corner screen.
  kBannerViewPositionBottomLeft,
  /// Bottom-right corner screen.
  kBannerViewPositionBottomRight
};

/// Displays banners ads.
class BannerView {
 public:

  /// Creates a new BannerView object that can be used to display an AdMob banner.
  ///
  /// @param parent The platform-specific UI element that will host the ad.
  /// @param ad_unit_id The ad unit ID to use when requesting ads.
  /// @param size The desired ad size for the banners.
  /// "hidden" state.
  BannerView(AdParent parent, const char* ad_unit_id, AdSize size);

  /// Destructor.
  ~BannerView();

  /// Begins an asynchronous request for an ad. If successful, the ad will
  /// automatically be displayed in the BannerView. The (@see BannerView::GetState
  /// method can be used to track the progress of the request.
  ///
  /// @param request Data specific to the request, which will be used
  /// by the server to select an ad.
  void LoadAd(AdRequest request);

  /// Hides the BannerView.
  void Hide();

  /// Shows the BannerView if it's not already visible.
  void Show();

  /// Pauses the BannerView. Should be called whenever the C++ engine pauses or the
  /// application loses focus.
  void Pause();

  /// Resumes the BannerView after pausing.
  void Resume();

  /// Cleans up and deallocates any resources used by the BannerView.
  void Destroy();

  /// Moves the BannerView so that its top-left corner is located at (x, y).
  void MoveTo(int x, int y);

  /// Moves the BannerView so that it's located at {@see position}.
  void MoveTo(BannerViewPosition position);

  /// Retrieves the BannerView's current onscreen size and location,
  ///
  /// @return The current size and location.
  BoundingBox GetBoundingBox();

  /// Returns the current state of the BannerView within its lifecycle (i.e.
  /// whether it's currently loading an ad, has failed, etc.).
  ///
  /// @return The current lifecycle state.
  BannerViewLifecycleState GetLifecycleState();

  /// Returns the current presentation state of the BannerView.
  ///
  /// @return The current presentation state.
  BannerViewPresentationState GetPresentationState();

private:
  jobject helper_;
};

}  // namespace admob
}  // namespace firebase

#endif
