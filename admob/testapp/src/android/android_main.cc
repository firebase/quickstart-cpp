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

#include <android/log.h>
#include <android_native_app_glue.h>
#include <stdarg.h>

#include "main.h"  // NOLINT

// This implementation is derived from http://github.com/google/fplutil

extern "C" int common_main(void* ad_parent);

static struct android_app* gAppState = nullptr;

// Process events pending on the main thread.
bool ProcessAndroidEvents(int msec) {
  struct android_poll_source* source = nullptr;
  int events;
  int looperId = ALooper_pollAll(msec, nullptr, &events,
                                 reinterpret_cast<void**>(&source));
  if (looperId >= 0 && source) {
    source->process(gAppState, source);
  }
  return gAppState->destroyRequested;
}

// Log a message that can be viewed in "adb logcat".
int LogMessage(const char* format, ...) {
  va_list list;
  int rc;
  va_start(list, format);
  rc = __android_log_vprint(ANDROID_LOG_INFO, FIREBASE_TESTAPP_NAME, format,
                            list);
  va_end(list);
  return rc;
}

// Get the JNI environment.
JNIEnv* GetJniEnv() {
  JavaVM* vm = gAppState->activity->vm;
  JNIEnv* env;
  jint result = vm->AttachCurrentThread(&env, nullptr);
  return result == JNI_OK ? env : nullptr;
}

// Get the activity.
jobject GetActivity() { return gAppState->activity->clazz; }

// Execute common_main(), flush pending events and finish the activity.
extern "C" void android_main(struct android_app* state) {
  int return_value;
  gAppState = state;
  static const char* argv[] = {FIREBASE_TESTAPP_NAME};
  return_value = common_main(static_cast<void*>(GetActivity()));
  (void)return_value;  // Ignore the return value.
  ProcessAndroidEvents(10);
  ANativeActivity_finish(state->activity);
  gAppState->activity->vm->DetachCurrentThread();
}
