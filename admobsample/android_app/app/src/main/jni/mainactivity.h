#include <jni.h>

#ifndef MAINACTIVITY_H
#define MAINACTIVITY_H

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL
Java_com_google_firebase_admob_admobcppapp_MainActivity_initializeGameEngine(
    JNIEnv *env, jobject instance);

JNIEXPORT void JNICALL
Java_com_google_firebase_admob_admobcppapp_GLRenderer_nativeOnSurfaceCreated(
    JNIEnv *env, jobject instance);

JNIEXPORT void JNICALL
Java_com_google_firebase_admob_admobcppapp_GLRenderer_nativeOnSurfaceChanged(
    JNIEnv *env, jobject instance, jint width, jint height);

JNIEXPORT void JNICALL
Java_com_google_firebase_admob_admobcppapp_GLRenderer_nativeOnDrawFrame(
    JNIEnv *env, jobject instance);

JNIEXPORT void JNICALL
Java_com_google_firebase_admob_admobcppapp_GLRenderer_nativeOnUpdate(
    JNIEnv *env, jobject instance);

JNIEXPORT void JNICALL
Java_com_google_firebase_admob_admobcppapp_MainActivity_registerTap(
    JNIEnv *env, jobject instance, jfloat x, jfloat y);

JNIEXPORT void JNICALL
Java_com_google_firebase_admob_admobcppapp_MainActivity_pauseGameEngine(
    JNIEnv *env, jobject instance);

JNIEXPORT void JNICALL
Java_com_google_firebase_admob_admobcppapp_MainActivity_resumeGameEngine(
    JNIEnv *env, jobject instance);

#ifdef __cplusplus
}
#endif
#endif  // MAINACTIVITY_H
