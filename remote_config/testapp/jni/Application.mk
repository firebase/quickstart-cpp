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

APP_PLATFORM:=android-14
# NOTE: Users can build against either armeabi-v7a or armeabi-v7a-hard.
APP_ABI:=armeabi-v7a-hard arm64-v8a x86 x86_64 mips
# armeabi and mips64 - Are broken when building NDK r11+ at the moment.
APP_STL:=gnustl_static  # NOTE: c++_static does not link at the moment.
APP_MODULES:=android_main
APP_CPPFLAGS+=-std=c++11
