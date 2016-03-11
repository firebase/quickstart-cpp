# Copyright 2016 Google Inc. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH:=$(call my-dir)/..

ifeq ($(FIREBASE_PACKAGE_PATH),)
$(error FIREBASE_PACKAGE_PATH must specify the Firebase package location.)
endif

# With Firebase libraries for the selected build configuration (ABI + STL)
# TODO(smiles): Re-enable STL when cl/115590224 is checked in.
FIREBASE_LIBRARY_PATH:=\
$(FIREBASE_PACKAGE_PATH)/android/$(TARGET_ARCH_ABI)# /$(APP_STL)

include $(CLEAR_VARS)
LOCAL_MODULE:=firebase_app
LOCAL_SRC_FILES:=$(FIREBASE_LIBRARY_PATH)/libapp.a
LOCAL_EXPORT_C_INCLUDES:=$(FIREBASE_PACKAGE_PATH)/include
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE:=firebase_invites
LOCAL_SRC_FILES:=$(FIREBASE_LIBRARY_PATH)/libinvites.a
LOCAL_STATIC_LIBRARIES:=firebase_app
LOCAL_EXPORT_C_INCLUDES:=$(FIREBASE_PACKAGE_PATH)/include
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE:=libmain_activity
LOCAL_SRC_FILES:=\
	$(LOCAL_PATH)/src/common_main.cc \
	$(LOCAL_PATH)/src/android/android_main.cc
LOCAL_STATIC_LIBRARIES:=\
	firebase_app \
	firebase_invites \
	android_native_app_glue
LOCAL_C_INCLUDES:=\
	$(NDK_ROOT)/sources/android/native_app_glue \
	$(LOCAL_PATH)/src
LOCAL_ARM_MODE:=arm
LOCAL_LDLIBS:=-llog -landroid
include $(BUILD_SHARED_LIBRARY)

$(call import-add-path,$(NDK_ROOT)/sources/android)
$(call import-module,android/native_app_glue)
