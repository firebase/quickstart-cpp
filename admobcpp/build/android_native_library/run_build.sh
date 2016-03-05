#!/bin/bash

for arch in armeabi armeabi-v7a arm64-v8a x86; do
  rm ../../libs/$arch/*.a
done

set -e

ndk-build NDK_PROJECT_PATH=$(pwd) APP_BUILD_SCRIPT=$(pwd)/Android.mk APP_STL=c++_static APP_ABI="armeabi armeabi-v7a arm64-v8a x86" clean
ndk-build NDK_PROJECT_PATH=$(pwd) APP_BUILD_SCRIPT=$(pwd)/Android.mk APP_STL=c++_static APP_ABI="armeabi armeabi-v7a arm64-v8a x86" APP_PLATFORM=android-9

for arch in armeabi armeabi-v7a arm64-v8a x86; do
  cp -f obj/local/$arch/*.a ../../libs/$arch
done
