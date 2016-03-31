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
#include <stdio.h>
#include <cassert>

#include "main.h"  // NOLINT

// This implementation is derived from http://github.com/google/fplutil

extern "C" int common_main(int argc, const char* argv[]);

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

// Vars that we need available for appending text to the log window:
class TextViewData {
 public:
  TextViewData()
      : text_view_obj_(0),
        text_view_class_(0),
        string_class_(0),
        text_view_append_(0) {}

  ~TextViewData() {
    JNIEnv* env = GetJniEnv();
    assert(env);
    env->DeleteLocalRef(text_view_obj_);
    text_view_obj_ = 0;
  }

  jobject text_view_obj() const { return text_view_obj_; }
  void set_text_view_obj(jobject text_view_obj) {
    text_view_obj_ = text_view_obj;
  }

  jclass text_view_class() const { return text_view_class_; }
  void set_text_view_class(jclass text_view_class) {
    text_view_class_ = text_view_class;
  }

  jclass string_class() const { return string_class_; }
  void set_string_class(jclass string_class) { string_class_ = string_class; }

  jmethodID text_view_append() const { return text_view_append_; }
  void set_text_view_append(jmethodID text_view_append) {
    text_view_append_ = text_view_append;
  }

 private:
  jobject text_view_obj_;
  jclass text_view_class_;
  jclass string_class_;
  jmethodID text_view_append_;
};
TextViewData text_view_data;

// Appends a string to the text view to be displayed.
// Warning - do not put log statements in here.  Since the log statement
// mirrors its output to this, trying to call LogMessage from this subroutine
// causes loops and problems.
void AppendTextViewText(const char* text) {
  // Abort if the text view hasn't been created yet.
  if (text_view_data.text_view_obj() == 0) {
    return;
  }
  JNIEnv* env = GetJniEnv();
  assert(env);

  jstring text_string = env->NewStringUTF(text);

  env->CallVoidMethod(text_view_data.text_view_obj(),
                      text_view_data.text_view_append(), text_string);

  env->DeleteLocalRef(text_string);
}

// Log a message that can be viewed in "adb logcat".
int LogMessage(const char* format, ...) {
  static const int kLineBufferSize = 100;
  char buffer[kLineBufferSize + 1];

  va_list list;
  int rc;
  va_start(list, format);
  int string_len = vsnprintf(buffer, kLineBufferSize, format, list);
  // append a linebreak to the buffer:
  buffer[string_len] = '\n';
  buffer[string_len + 1] = '\0';

  AppendTextViewText(buffer);
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

// Create a text view, inside of a scroll view, inside of a linear layout
// and display them on screen.  This is equivalent to the following
// java code:
//  private TextView text_view = new TextView(this);
//  LinearLayout linear_layout = new LinearLayout(this);
//  linear_layout.addView(text);
//  Window window = getWindow();
//  window.takeSurface(null);
//  window.setContentView(linear_layout);
void CreateJavaTextView(android_app* state) {
  JNIEnv* env = GetJniEnv();
  assert(env);
  // cache these off for later:
  text_view_data.set_string_class(env->FindClass("java/lang/String"));
  text_view_data.set_text_view_class(env->FindClass("android/widget/TextView"));
  jclass scroll_view_class = env->FindClass("android/widget/ScrollView");

  text_view_data.set_text_view_append(
      env->GetMethodID(text_view_data.text_view_class(), "append",
                       "(Ljava/lang/CharSequence;)V"));

  // Construct a linear layout.
  jclass linear_layout_class = env->FindClass("android/widget/LinearLayout");
  jmethodID linear_layout_constructor = env->GetMethodID(
      linear_layout_class, "<init>", "(Landroid/content/Context;)V");
  jobject linear_layout_obj = env->NewObject(
      linear_layout_class, linear_layout_constructor, state->activity->clazz);

  // Construct a scrollview.
  jmethodID scroll_view_constructor = env->GetMethodID(
      scroll_view_class, "<init>", "(Landroid/content/Context;)V");
  jobject scroll_view_obj = env->NewObject(
      scroll_view_class, scroll_view_constructor, state->activity->clazz);

  // Construct a text view
  jmethodID text_view_constructor =
      env->GetMethodID(text_view_data.text_view_class(), "<init>",
                       "(Landroid/content/Context;)V");

  text_view_data.set_text_view_obj(
      env->NewObject(text_view_data.text_view_class(), text_view_constructor,
                     state->activity->clazz));

  // Add the text view to the scroll view
  // and the scroll view to the linear layout.
  jmethodID view_add_view = env->GetMethodID(linear_layout_class, "addView",
                                             "(Landroid/view/View;)V");
  env->CallVoidMethod(linear_layout_obj, view_add_view, scroll_view_obj);
  env->CallVoidMethod(scroll_view_obj, view_add_view,
                      text_view_data.text_view_obj());

  // Add the linear layout to the view.
  jclass activity_class = env->FindClass("android/app/Activity");
  jmethodID activity_get_window =
      env->GetMethodID(activity_class, "getWindow", "()Landroid/view/Window;");
  jobject window_obj =
      env->CallObjectMethod(state->activity->clazz, activity_get_window);

  // Take control of the window and display the linearlayout in it.
  jclass window_class = env->FindClass("android/view/Window");

  jmethodID window_take_surface = env->GetMethodID(
      window_class, "takeSurface", "(Landroid/view/SurfaceHolder$Callback2;)V");
  env->CallVoidMethod(window_obj, window_take_surface, 0);

  jmethodID window_set_content_view = env->GetMethodID(
      window_class, "setContentView", "(Landroid/view/View;)V");
  env->CallVoidMethod(window_obj, window_set_content_view, linear_layout_obj);

  // Clean up our local references.
  env->DeleteLocalRef(window_obj);
  env->DeleteLocalRef(scroll_view_obj);
  env->DeleteLocalRef(linear_layout_obj);
}

// Execute common_main(), flush pending events and finish the activity.
extern "C" void android_main(struct android_app* state) {
  int return_value;
  gAppState = state;
  static const char* argv[] = {FIREBASE_TESTAPP_NAME};
  CreateJavaTextView(state);
  return_value = common_main(1, argv);
  (void)return_value;  // Ignore the return value.
  ProcessAndroidEvents(10);
  ANativeActivity_finish(state->activity);
  gAppState->activity->vm->DetachCurrentThread();
}
