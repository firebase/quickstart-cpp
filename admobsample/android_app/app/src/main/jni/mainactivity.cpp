#include <android/log.h>
#include <jni.h>

#include "cpp_main.h"
#include "mainactivity.h"

/**
 * An instance of GLDrawer to use to draw on the GLSurfaceView.
 */
namespace {
CPPMain *engine;
}

JNIEXPORT void JNICALL
Java_com_google_firebase_admob_admobcppapp_MainActivity_initializeGameEngine(
    JNIEnv *env, jobject instance) {
  __android_log_print(ANDROID_LOG_INFO, "GMACPP",
                      "mainactivity::initialize(env, instance)");
  engine = new CPPMain();
  engine->Initialize(env, instance);
}

JNIEXPORT void JNICALL
Java_com_google_firebase_admob_admobcppapp_GLRenderer_nativeOnSurfaceCreated(
    JNIEnv *env, jobject instance) {
  __android_log_print(ANDROID_LOG_INFO, "GMACPP",
                      "mainactivity::OnSurfaceCreated()");
  if (engine) {
    engine->onSurfaceCreated();
  }
}

JNIEXPORT void JNICALL
Java_com_google_firebase_admob_admobcppapp_GLRenderer_nativeOnSurfaceChanged(
    JNIEnv *env, jobject instance, jint width, jint height) {
  __android_log_print(ANDROID_LOG_INFO, "GMACPP",
                      "mainactivity::OnSufaceChanged(%d, %d)", width, height);
  if (engine) {
    engine->onSurfaceChanged(width, height);
  }
}

JNIEXPORT void JNICALL
Java_com_google_firebase_admob_admobcppapp_GLRenderer_nativeOnDrawFrame(
    JNIEnv *env, jobject instance) {
  //__android_log_print(ANDROID_LOG_INFO, "GMACPP",
  //"mainactivity::OnDrawFrame()");
  if (engine) {
    engine->onDrawFrame();
  }
}

JNIEXPORT void JNICALL
Java_com_google_firebase_admob_admobcppapp_GLRenderer_nativeOnUpdate(
    JNIEnv *env, jobject instance) {
  //__android_log_print(ANDROID_LOG_INFO, "GMACPP", "mainactivity::OnUpdate()");
  if (engine) {
    engine->onUpdate();
  }
}

JNIEXPORT void JNICALL
Java_com_google_firebase_admob_admobcppapp_MainActivity_registerTap(
    JNIEnv *env, jobject instance, jfloat x, jfloat y) {
  if (engine) {
    engine->onTap(x, y);
  }
}

JNIEXPORT void JNICALL
Java_com_google_firebase_admob_admobcppapp_MainActivity_pauseGameEngine(
    JNIEnv *env, jobject instance) {
  __android_log_print(ANDROID_LOG_INFO, "GMACPP", "mainactivity::Pause()");
  if (engine) {
    engine->Pause();
  }
}

JNIEXPORT void JNICALL
Java_com_google_firebase_admob_admobcppapp_MainActivity_resumeGameEngine(
    JNIEnv *env, jobject instance) {
  __android_log_print(ANDROID_LOG_INFO, "GMACPP", "mainactivity::Resume()");
  if (engine) {
    engine->Resume();
  }
}
