/*
 * Copyright 2016 Google, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CPP_MAIN_H_
#define CPP_MAIN_H_

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <OpenGLES/ES2/gl.h>
#else
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <jni.h>
#endif

#include "../../../../../../admobcpp/include/banner_view.h"
#include "../../../../../../admobcpp/include/interstitial_ad.h"
#include "../../../../../../admobcpp/include/types.h"
#include "button.h"

#ifndef __cplusplus
#error Header file supports C++ only
#endif  // __cplusplus

class CPPMain {
  static const int kNumberOfButtons = 4;

 public:
  CPPMain();

#ifdef __APPLE__
  void Initialize();
#else
  void Initialize(JNIEnv* env, jobject activity);
#endif
  void onSurfaceCreated();
  void onSurfaceChanged(int width, int height);
  void onDrawFrame();
  void onUpdate();
  void onTap(float x, float y);
  void Pause();
  void Resume();

 private:
  firebase::admob::AdRequest createRequest();

  firebase::admob::BannerView* banner_view_;
  firebase::admob::InterstitialAd* interstitial_ad_;

  GLuint vertex_shader_;
  GLuint fragment_shader_;
  GLuint shader_program_;
  int height_;
  int width_;
  Button button_list_[kNumberOfButtons];
};

#endif  // CPP_MAIN_H_
