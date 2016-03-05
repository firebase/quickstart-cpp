/* TODO: Copyright */

#include <cstddef>
#include <cstdarg>

#include <assert.h>
#include <jni.h>

#include "../../include/types.h"
#include "../../include/admob.h"
#include "../../include/banner_view.h"
#include "logging.h"

namespace firebase {
namespace admob {

  // Used to populate an array of MethodNameSignature.
  #define METHOD_NAME_SIGNATURE(id, name, signature) \
    { name, signature }

  // Used to populate an enum of method idenfitiers.
  #define METHOD_ID(id, name, signature) k##id

  // Used to setup the cache of BannerViewHelper class method IDs to reduce
  // time spent looking up methods by string.
  // clang-format off
  #define BANNERVIEWHELPER_METHODS(X)                                           \
    X(Constructor, "<init>", "(Landroid/app/Activity;)V"),                      \
    X(CreateDate, "createDate", "(III)Ljava/util/Date;"),                       \
    X(Destroy, "destroy", "()V"),                                               \
    X(Pause, "pause", "()V"),                                                   \
    X(Resume, "resume", "()V"),                                                 \
    X(Show, "show", "()V"),                                                     \
    X(Hide, "hide", "()V"),                                                     \
    X(GetWidth, "getWidth", "()I"),                                             \
    X(GetHeight, "getHeight", "()I"),                                           \
    X(GetX, "getX", "()I"),                                                     \
    X(GetY, "getY", "()I"),                                                     \
    X(MoveToPosition, "moveTo", "(I)V"),                                        \
    X(MoveToXY, "moveTo", "(II)V"),                                             \
    X(LoadAd, "loadAd", "(Lcom/google/android/gms/ads/AdRequest;)V"),           \
    X(GetLifecycleState, "getLifecycleState", "()I"),                           \
    X(GetPresentationState, "getPresentationState", "()I")
  // clang-format on

  // clang-format off
  #define ADREQUESTBUILDER_METHODS(X)                                           \
    X(Constructor, "<init>", "()V"),                                            \
    X(Build, "build", "()Lcom/google/android/gms/ads/AdRequest;"),              \
    X(AddKeyword, "addKeyword", "(Ljava/lang/String;)Lcom/google/android/gms/ads/AdRequest$Builder;"),                        \
    X(AddTestDevice, "addTestDevice", "(Ljava/lang/String;)Lcom/google/android/gms/ads/AdRequest$Builder;"),                  \
    X(SetBirthday, "setBirthday", "(Ljava/util/Date;)Lcom/google/android/gms/ads/AdRequest$Builder;"),                       \
    X(SetGender, "setGender", "(I)Lcom/google/android/gms/ads/AdRequest$Builder;"),                                           \
    X(TagForChildDirectedTreatment, "tagForChildDirectedTreatment", "(Z)Lcom/google/android/gms/ads/AdRequest$Builder;")
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

METHOD_LOOKUP_DECLARATION(bannerviewhelper,
                          "com/google/android/gms/ads/cpphelpers/BannerViewHelper",
                          BANNERVIEWHELPER_METHODS);

METHOD_LOOKUP_DECLARATION(adrequestbuilder,
                          "com/google/android/gms/ads/AdRequest$Builder",
                          ADREQUESTBUILDER_METHODS);

BannerView::BannerView(AdParent parent, const char* ad_unit_id, AdSize size) {
  // TODO: Threadsafe this.
  JNIEnv* env = ::firebase::admob::GetJNI();
  adrequestbuilder::CacheMethodIds(env);
  bannerviewhelper::CacheMethodIds(env);

  jobject helper_ref = env->NewObject(
      bannerviewhelper::GetClass(),
      bannerviewhelper::GetMethodId(bannerviewhelper::kConstructor),
      parent);
  assert(helper_ref);
  helper_ = env->NewGlobalRef(helper_ref);
  assert(helper_);
}

BannerView::~BannerView() {
    ::firebase::admob::GetJNI()->DeleteLocalRef(helper_);
}

void BannerView::LoadAd(AdRequest request) {
  JNIEnv* env = ::firebase::admob::GetJNI();

  jobject ad_request_builder = env->NewObject(
      adrequestbuilder::GetClass(),
      adrequestbuilder::GetMethodId(adrequestbuilder::kConstructor));

  // Gender.
  env->CallObjectMethod(
      ad_request_builder,
      adrequestbuilder::GetMethodId(adrequestbuilder::kSetGender),
      (int)request.gender);

  // Child-drected treatment.
  if (request.tagged_for_child_directed_treatment
    != kChildDirectedTreatmentStateUnknown) {
    env->CallObjectMethod(
        ad_request_builder,
        adrequestbuilder::GetMethodId(
          adrequestbuilder::kTagForChildDirectedTreatment),
        (request.tagged_for_child_directed_treatment
          == kChildDirectedTreatmentStateTagged));
  }

  // Test devices.
  for (int i = 0; i < request.test_device_id_count; i++) {
    jstring test_device_str = env->NewStringUTF(request.test_device_ids[i]);
    env->CallObjectMethod(
        ad_request_builder,
        adrequestbuilder::GetMethodId(adrequestbuilder::kAddTestDevice),
        test_device_str);
    env->DeleteLocalRef(test_device_str);
  }

  // Keywords.
  for (int i = 0; i < request.keyword_count; i++) {
    jstring keyword_str = env->NewStringUTF(request.keywords[i]);
    env->CallObjectMethod(
        ad_request_builder,
        adrequestbuilder::GetMethodId(adrequestbuilder::kAddKeyword),
        keyword_str);
    env->DeleteLocalRef(keyword_str);
  }

  // Extras?
  for (int i = 0; i < request.extras_count; i++) {
    // jstring keyString = env->NewStringUTF(request.extras[i].key);
    // jstring valueString = env->NewStringUTF(request.extras[i].value);
    // env->CallObjectMethod(
    //     ad_request_builder,
    //     adrequestbuilder::GetMethodId(adrequestbuilder::kAddExtra),
    //     utfString);
    // env->DeleteLocalRef(utfString);
  }

  // Birthday
  jobject date_ref = env->CallObjectMethod(
      helper_,
      bannerviewhelper::GetMethodId(bannerviewhelper::kCreateDate),
      (jint)request.birthday_year,
      (jint)request.birthday_month,
      (jint)request.birthday_day);

  if (date_ref != NULL) {
      env->CallObjectMethod(
        ad_request_builder,
        adrequestbuilder::GetMethodId(adrequestbuilder::kSetBirthday),
        date_ref);
  }

  // Build request and load ad.
  jobject ad_request = env->CallObjectMethod(
      ad_request_builder,
      adrequestbuilder::GetMethodId(adrequestbuilder::kBuild));

  env->CallVoidMethod(
    helper_,
    bannerviewhelper::GetMethodId(bannerviewhelper::kLoadAd),
    ad_request);
}

void BannerView::Hide() {
  ::firebase::admob::GetJNI()->CallVoidMethod(helper_,
      bannerviewhelper::GetMethodId(bannerviewhelper::kHide));
}

void BannerView::Show() {
  ::firebase::admob::GetJNI()->CallVoidMethod(helper_,
      bannerviewhelper::GetMethodId(bannerviewhelper::kShow));
}

void BannerView::Pause() {
  ::firebase::admob::GetJNI()->CallVoidMethod(helper_,
      bannerviewhelper::GetMethodId(bannerviewhelper::kPause));
}

void BannerView::Resume() {
  ::firebase::admob::GetJNI()->CallVoidMethod(helper_,
      bannerviewhelper::GetMethodId(bannerviewhelper::kResume));
}

void BannerView::Destroy() {
  ::firebase::admob::GetJNI()->CallVoidMethod(helper_,
      bannerviewhelper::GetMethodId(bannerviewhelper::kDestroy));
}

void BannerView::MoveTo(int x, int y) {
  ::firebase::admob::GetJNI()->CallVoidMethod(helper_,
      bannerviewhelper::GetMethodId(bannerviewhelper::kMoveToXY),
      x,
      y);
}

void BannerView::MoveTo(BannerViewPosition position) {
  ::firebase::admob::GetJNI()->CallVoidMethod(helper_,
      bannerviewhelper::GetMethodId(bannerviewhelper::kMoveToPosition),
      (int)position);
}

BoundingBox BannerView::GetBoundingBox() {
  BoundingBox box;

  jint width = ::firebase::admob::GetJNI()->CallIntMethod(helper_,
      bannerviewhelper::GetMethodId(bannerviewhelper::kGetWidth));
  jint height = ::firebase::admob::GetJNI()->CallIntMethod(helper_,
      bannerviewhelper::GetMethodId(bannerviewhelper::kGetHeight));
  jint x = ::firebase::admob::GetJNI()->CallIntMethod(helper_,
      bannerviewhelper::GetMethodId(bannerviewhelper::kGetX));
  jint y = ::firebase::admob::GetJNI()->CallIntMethod(helper_,
      bannerviewhelper::GetMethodId(bannerviewhelper::kGetY));

  return box;
}

BannerViewLifecycleState BannerView::GetLifecycleState() {
  jint state = ::firebase::admob::GetJNI()->CallIntMethod(helper_,
      bannerviewhelper::GetMethodId(bannerviewhelper::kGetLifecycleState));
  return static_cast<BannerViewLifecycleState>(state);
}

BannerViewPresentationState BannerView::GetPresentationState() {
  jint state = ::firebase::admob::GetJNI()->CallIntMethod(helper_,
      bannerviewhelper::GetMethodId(bannerviewhelper::kGetPresentationState));
  return static_cast<BannerViewPresentationState>(state);
}

}  // namespace admob
}  // namespace firebase
