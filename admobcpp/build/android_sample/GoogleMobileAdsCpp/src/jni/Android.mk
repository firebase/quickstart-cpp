LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

MY_BASE_PATH := $(LOCAL_PATH)/../..

LOCAL_SRC_FILES := $(MY_BASE_PATH)/src/common/google_mobile_ads.cpp

LOCAL_MODULE := GoogleMobileAds

include $(BUILD_STATIC_LIBRARY)
