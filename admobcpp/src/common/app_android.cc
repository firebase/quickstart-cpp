
// Copyright 2016 Google Inc. All Rights Reserved.

#include "../../include/app.h"

#include <memory>

namespace firebase {

std::unique_ptr<App> g_app;

App::App() : jni_env_(nullptr), activity_(nullptr) {}
App::~App() {
  jni_env_->DeleteGlobalRef(activity_);
  activity_ = nullptr;
  g_app.release();
}

App* App::Create(JNIEnv* jni_env, jobject activity) {
  assert(!g_app.get());
  g_app.reset(new App);
  g_app->jni_env_ = jni_env;
  g_app->activity_ = jni_env->NewGlobalRef(activity);
  return g_app.get();
}

App& App::Get() { return *g_app.get(); }

}  // namespace firebase {
