LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

MY_BASE_PATH := $(LOCAL_PATH)/../..

LOCAL_SRC_FILES := $(MY_BASE_PATH)/src/common/admob.cpp
LOCAL_SRC_FILES += $(MY_BASE_PATH)/src/common/banner_view.cpp
LOCAL_SRC_FILES += $(MY_BASE_PATH)/src/common/interstitial_ad.cpp
LOCAL_SRC_FILES += $(MY_BASE_PATH)/src/common/app_android.cc

LOCAL_MODULE := FirebaseAdMob

include $(BUILD_STATIC_LIBRARY)
