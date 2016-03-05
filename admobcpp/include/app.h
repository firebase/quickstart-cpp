// Copyright 2016 Google Inc. All Rights Reserved.

#ifndef FIREBASE_APP_H
#define FIREBASE_APP_H

#ifdef __ANDROID__
#include <jni.h>
#endif  // __ANDROID__

namespace firebase {

class App {
 public:
  ~App();

#ifdef __ANDROID__
  /// Get the JNI environment.
  JNIEnv* jni_env() const { return jni_env_; }
  /// Get the activity used to start the application.
  jobject activity() const { return activity_; }
#endif  // __ANDROID__

#ifdef __ANDROID__
  /// Create the App object.
  ///
  /// The arguments to App::Create() are platform specific so the caller must
  /// do something like this:
  /// \code
  /// #if defined(__ANDROID__)
  /// firebase::App::Create(jni_env, activity);
  /// #elif defined(__IOS__)
  /// firebase::App::Create();  // TODO: Update?
  /// #else
  /// firebase::App::Create();
  /// #endif
  /// \endcode
  ///
  /// @param jni_env JNIEnv pointer.
  /// @param activity Activity used to start the application.
  static App* Create(JNIEnv* jni_env, jobject activity);
// TODO: May want a convienience function that takes android_app (native
// activity).
#else
  /// Create the App object.
  static App* Create();
#endif  // __ANDROID__

  /// Get a previously created App object.
  static App& Get();

 private:
  App();

#ifdef __ANDROID__
  JNIEnv* jni_env_;
  jobject activity_;
#endif  // __ANDROID__
};

}  // namespace firebase

#endif  // FIREBASE_APP_H
