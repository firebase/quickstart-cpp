// Copyright 2016 Google Inc. All Rights Reserved.

#ifndef FIREBASE_ADMOB_H_
#define FIREBASE_ADMOB_H_

#include "types.h"
#include "app.h"

namespace firebase {
namespace admob {

/// Initializes AdMob via Firebase.
///
/// @param app The Firebase app for which to initialize mobile ads.
void Initialize(const ::firebase::App& app);

#ifdef __ANDROID__
  /// Initialize AdMob without Firebase.
  ///
  /// The arguments to Initialize() are platform specific so the caller must
  /// do something like this:
  /// \code
  /// #if defined(__ANDROID__)
  /// firebase::admob::Initialize(jni_env, activity);
  /// #elif defined(__IOS__)
  /// firebase::admob::Initialize();
  /// #endif
  /// \endcode
  ///
  /// @param jni_env JNIEnv pointer.
  /// @param activity Activity used to start the application.
void Initialize(JNIEnv* jni_env, jobject activity);
#else
/// Initialize AdMob without Firebase.
void Initialize();
#endif  // __ANDROID__

const ::firebase::App* GetApp();

#ifdef __ANDROID__
JNIEnv* GetJNI();
#endif



}  // namespace admob
}  // namespace firebase

#endif
