// Copyright 2016 Google Inc. All Rights Reserved.

#ifndef FIREBASE_ADMOB_TYPES_H_
#define FIREBASE_ADMOB_TYPES_H_

namespace firebase {
namespace admob {

// To create an AdView or InterstitialAd, the publisher will provide a context
// or UIViewController to host it.
#ifdef __ANDROID__
#include <jni.h>
  typedef jobject AdParent; // An Android Activity from Java.
#else
  @import UIKit;
  typedef UIViewController* AdParent; // iOS ViewController used in presenting.
#endif

/// Gender information used as part of the AdRequest struct.
enum Gender {
  /// The gender of the current user is unknown or unspecified by the publisher.
  kGenderUnknown = 0,
  /// The current user is known to be male.
  kGenderMale,
  /// The current user is known to be female.
  kGenderFemale
};

/// Indicates whether an ad request is considered tagged for child-directed
/// treatment.
enum ChildDirectedTreatmentState {
  /// The child-directed status for the request is not indicated.
  kChildDirectedTreatmentStateUnknown = 0,
  /// The request is tagged for child-directed treatment.
  kChildDirectedTreatmentStateTagged,
  /// The request is not tagged for child-directed treatement.
  kChildDirectedTreatmentStateNotTagged
};

/// Types of ad sizes.
enum AdSizeType {
  kStandard = 0
};

/// Generic Key-Value container used for the AdRequests "extra" values.
struct KeyValuePair {
  const char *key;
  const char *value;
};

/// The information needed for an AdView or InterstitialAd to request an ad.
struct AdRequest {
  const char **test_device_ids;
  unsigned int test_device_id_count;
  const char **keywords;
  unsigned int keyword_count;
  KeyValuePair **extras;
  unsigned int extras_count;
  int birthday_day;
  int birthday_month;
  int birthday_year;
  Gender gender;
  ChildDirectedTreatmentState tagged_for_child_directed_treatment;
};

/// Represents the screen location and dimensions of a BannerView once it has
/// been initalized. The coordinates for the BannerView's top-left corner are
/// contained in {@see point} and its width and height are contained in
/// {@see size}.
struct BoundingBox {
  int width;
  int height;
  int x;
  int y;
};

/// An ad size value to be used in requesting ads.
struct AdSize {
  AdSizeType adSizeType;
  int width;
  int height;
};

} // namespace admob
} // namespace firebase

#endif
