/* TODO: Copyright */

#include <cstddef>
#include <cstdarg>

#include <assert.h>
#include <jni.h>

#include "../../include/admob.h"
#include "../../include/types.h"
#include "logging.h"

namespace firebase {
namespace admob {

// Used to populate an array of MethodNameSignature.
#define METHOD_NAME_SIGNATURE(id, name, signature) \
  { name, signature }

// Used to populate an enum of method idenfitiers.
#define METHOD_ID(id, name, signature) k##id

// Used to setup the cache of Measurement class method IDs to reduce time spent
// looking up methods by string.
// clang-format off
#define ADMOBHELPER_METHODS(X)                                                \
  X(SetAppVolume, "setAppVolume", "(F)V")
// clang-format on

// Creates a local namespace which caches class method IDs.
// clang-format off
#define METHOD_LOOKUP_DECLARATION(namespace_identifier,                       \
                                  class_name, method_descriptor_macro)        \
  namespace namespace_identifier {                                            \
                                                                              \
  enum Method {                                                               \
    method_descriptor_macro(METHOD_ID),                                       \
    kMethodCount                                                              \
  };                                                                          \
                                                                              \
  static const char* kClassName = class_name;                                 \
                                                                              \
  static const MethodNameSignature kMethodSignatures[] = {                    \
    method_descriptor_macro(METHOD_NAME_SIGNATURE)                            \
  };                                                                          \
                                                                              \
  static jmethodID g_method_ids[kMethodCount];                                \
                                                                              \
  static jclass g_class = nullptr;                                            \
                                                                              \
  /* Find and hold a reference to this namespace's class. */                  \
  jclass CacheClass(JNIEnv *env) {                                            \
    if (!g_class) {                                                           \
      jclass local_class = env->FindClass(kClassName);                        \
      assert(local_class);                                                    \
      g_class = static_cast<jclass>(env->NewGlobalRef(local_class));          \
      assert(g_class);                                                        \
      env->DeleteLocalRef(local_class);                                       \
    }                                                                         \
    return g_class;                                                           \
  }                                                                           \
                                                                              \
  /* Get the cached class associated with this namespace. */                  \
  jclass GetClass() { return g_class; }                                       \
                                                                              \
  /* Release the cached class reference. */                                   \
  void ReleaseClass(JNIEnv *env) {                                            \
    assert(g_class);                                                          \
    env->DeleteGlobalRef(g_class);                                            \
    g_class = nullptr;                                                        \
  }                                                                           \
                                                                              \
  /* See LookupMethodIds() */                                                 \
  void CacheMethodIds(JNIEnv *env) {                                          \
    LookupMethodIds(env, CacheClass(env), kMethodSignatures, kMethodCount,    \
                    g_method_ids, kClassName);                                \
  }                                                                           \
                                                                              \
  /* Lookup a method ID using a Method enum value. */                         \
  jmethodID GetMethodId(Method method) {                                      \
    assert(method < kMethodCount);                                            \
    jmethodID method_id = g_method_ids[method];                               \
    assert(method_id);                                                        \
    return method_id;                                                         \
  }                                                                           \
                                                                              \
  }  // namespace namespace_identifier
  // clang-format on

// Name and signature of a class method.
struct MethodNameSignature {
  const char* name;
  const char* signature;
};

static JavaVM* g_java_vm = nullptr;
static JNIEnv* g_jni_env = nullptr;
static const ::firebase::App* g_app = nullptr;

static void LookupMethodIds(JNIEnv* env, jclass clazz,
                            const MethodNameSignature* method_name_signatures,
                            size_t number_of_method_name_signatures,
                            jmethodID* method_ids, const char* class_name) {
  assert(method_name_signatures);
  assert(number_of_method_name_signatures > 0);
  assert(method_ids);
  LOG("Looking up methods for %s", class_name);
  for (size_t i = 0; i < number_of_method_name_signatures; ++i) {
    const auto& method = method_name_signatures[i];
    method_ids[i] = env->GetMethodID(clazz, method.name, method.signature);
    // LOG("Method %s.%s (signature '%s') 0x%08x", class_name, method.name,
    //     method.signature, reinterpret_cast<int>(method_ids[i]));
    assert(method_ids[i]);
  }
}

METHOD_LOOKUP_DECLARATION(admobhelper,
                          "com/google/android/gms/ads/cpphelpers/AdMobHelper",
                          ADMOBHELPER_METHODS);

void Initialize(const ::firebase::App& app) {
  g_jni_env = app.jni_env();
  g_jni_env->GetJavaVM(&g_java_vm);
  g_app = &app;
  admobhelper::CacheMethodIds(g_jni_env);
}

void Initialize(JNIEnv* env, jobject activity) {
  g_jni_env = env;
  g_jni_env->GetJavaVM(&g_java_vm);
  admobhelper::CacheMethodIds(g_jni_env);
}

const ::firebase::App* GetApp() {
  if (!g_app) {
      LOG("App is being queried before ::Initialize()!");
  }
  return g_app;
}

JNIEnv* GetJNI() {
  if (!g_java_vm) {
      LOG("JNI is being queried before ::Initialize()!");
  }

  JNIEnv *env;
  g_java_vm->AttachCurrentThread( &env, NULL );

  return env;
}

}  // namespace admob
}  // namespace firebase



// #include <cstddef>
//
// #include "../../include/google_mobile_ads.h"
//
// namespace firebase {
// namespace mobileads {
//
// GoogleMobileAds::GoogleMobileAds() {
//     LOGI("GoogleMobileAds::GoogleMobileAds()");
// }
//
// void GoogleMobileAds::initialize(JNIEnv *env) {
//     LOGI("GoogleMobileAds::initialize(env)");
//
//     // JNIEnv *env;
//     // engine->app->activity->vm->AttachCurrentThread( &env, NULL );
//
//     jclass gmaClass = env->FindClass("com/google/android/gms/ads/cpphelpers/GoogleMobileAds");
//
//     if (gmaClass != NULL) {
//       LOGI("**** Found Helper!");
//     } else {
//       LOGI("**** Couldn't find Helper!");
//     }
//
//     jmethodID gmaConstructorMethod = env->GetMethodID(gmaClass, "<init>", "()V");
//     jmethodID gmaInitializeMethod = env->GetMethodID(gmaClass, "initialize", "()Ljava/lang/String;");
//
//     if (gmaInitializeMethod != NULL && gmaConstructorMethod != NULL) {
//       LOGI("**** Found methods!");
//     } else {
//       LOGI("**** Couldn't find methods!");
//     }
//
//     jobject gmaInstance = env->NewObject(gmaClass, gmaConstructorMethod);
//
//     if (gmaInstance != NULL) {
//       LOGI("**** Found instance!");
//     } else {
//       LOGI("**** Couldn't find instance!");
//     }
//
//     jstring initializeReturnVal = (jstring)(env->CallObjectMethod(gmaInstance, gmaInitializeMethod));
//
//     if (gmaInstance != NULL) {
//       LOGI("**** Found instance!");
//     } else {
//       LOGI("**** Couldn't find instance!");
//     }
//
//     const char *strReturn = env->GetStringUTFChars(initializeReturnVal, 0);
//     LOGI("%s", strReturn);
//
//     // jobject nativeActivity = engine->app->activity->clazz;
//     // jclass acl = env->GetObjectClass(nativeActivity);
//     // jmethodID getClassLoader = env->GetMethodID(acl, "getClassLoader", "()Ljava/lang/ClassLoader;");
//     // jobject cls = env->CallObjectMethod(nativeActivity, getClassLoader);
//     // jclass classLoader = env->FindClass("java/lang/ClassLoader");
//     // jmethodID findClass = env->GetMethodID(classLoader, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
//     // jstring strClassName = env->NewStringUTF("com.sample.SomeClass");
//     // jclass localClass = (jclass)(env->CallObjectMethod(cls, findClass, strClassName));
//     // env->DeleteLocalRef(strClassName);
//     //
//     // jclass globalClass = (jclass)(env->NewGlobalRef(localClass));
//     // jmethodID constructorMethod = env->GetMethodID(globalClass, "<init>", "()V");
//     // jmethodID doStuffMethod = env->GetMethodID(globalClass, "DoStuff", "()V");
//     // jmethodID showAdMethod = env->GetMethodID(globalClass, "ShowAd", "(Landroid/app/Activity;)V");
//     // jobject mySomeClass = env->NewObject(globalClass, constructorMethod);
//     // env->CallVoidMethod(mySomeClass, doStuffMethod);
//     // env->CallVoidMethod(mySomeClass, showAdMethod, nativeActivity);
//     //
//     //
//     // if (constructorMethod != NULL) {
//     //   LOGW("**** NOT NULL!");
//     // } else {
//     //   LOGW("**** NULL!");
//     // }
//     //
//     // if (env->ExceptionCheck()) { env->ExceptionDescribe(); }
// }
//
// }  // namespace mobileads
// }  // namespace firebase
