#/****************************************************************************
  # Copyright 2020 Google Inc. All rights reserved.
  #
  # Licensed under the Apache License, Version 2.0 (the "License");
  # you may not use this file except in compliance with the License.
  # You may obtain a copy of the License at
  #
  # http://www.apache.org/licenses/LICENSE-2.0
  #
  # Unless required by applicable law or agreed to in writing, software
  # distributed under the License is distributed on an "AS IS" BASIS,
  # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  # See the License for the specific language governing permissions and
  # limitations under the License.
#/****************************************************************************

#/****************************************************************************
  # Copyright (c) 2013-2014 cocos2d-x.org
  # Copyright (c) 2015-2017 Chukong Technologies Inc.
  #
  # http://www.cocos2d-x.org
  #
  # Permission is hereby granted, free of charge, to any person obtaining a copy
  # of this software and associated documentation files (the "Software"), to deal
  # in the Software without restriction, including without limitation the rights
  # to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  # copies of the Software, and to permit persons to whom the Software is
  # furnished to do so, subject to the following conditions:

  # The above copyright notice and this permission notice shall be included in
  # all copies or substantial portions of the Software.

  # THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  # IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  # FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  # AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  # LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  # OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  # THE SOFTWARE.
# ****************************************************************************/

cmake_minimum_required(VERSION 3.6)

set(APP_NAME tic_tac_toe_demo)

project(${APP_NAME})

# User settings for Firebase samples.
# Path to Firebase SDK.
# Try to read the path to the Firebase C++ SDK from an environment variable.
if (NOT "$ENV{FIREBASE_CPP_SDK_DIR}" STREQUAL "")
  set(DEFAULT_FIREBASE_CPP_SDK_DIR "$ENV{FIREBASE_CPP_SDK_DIR}")
else()
  set(DEFAULT_FIREBASE_CPP_SDK_DIR "firebase_cpp_sdk")
endif()
if ("${FIREBASE_CPP_SDK_DIR}" STREQUAL "")
  set(FIREBASE_CPP_SDK_DIR ${DEFAULT_FIREBASE_CPP_SDK_DIR})
endif()
if(NOT EXISTS ${FIREBASE_CPP_SDK_DIR})
  message(FATAL_ERROR "The Firebase C++ SDK directory does not exist: ${FIREBASE_CPP_SDK_DIR}. See the readme.md for more information")
endif()

# Build a desktop application.
# Windows runtime mode, either MD or MT depending on whether you are using
# /MD or /MT. For more information see:
# https://msdn.microsoft.com/en-us/library/2kzt1wy3.aspx
set(MSVC_RUNTIME_MODE MD)

if(APPLE)
  set(ADDITIONAL_LIBS gssapi_krb5 pthread "-framework CoreFoundation" "-framework Foundation" "-framework GSS"  "-framework Security")
elseif(MSVC)
  set(ADDITIONAL_LIBS advapi32 ws2_32 crypt32 iphlpapi psapi userenv)
else()
  set(ADDITIONAL_LIBS pthread)
endif()

# If a config file is present, copy it into the binary location so that it's
# possible to create the default Firebase app.
set(FOUND_JSON_FILE FALSE)
foreach(config "google-services-desktop.json" "google-services.json")
  if (EXISTS ${config})
    add_custom_command(
      TARGET ${APP_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy
      ${config} $<TARGET_FILE_DIR:${APP_NAME}>)
    set(FOUND_JSON_FILE TRUE)
    break()
  endif()
endforeach()
if(NOT FOUND_JSON_FILE)
  message(WARNING "Failed to find either google-services-desktop.json or google-services.json. See the readme.md for more information.")
endif()
#Target name change to cocos
# Add the Firebase libraries to the target using the function from the SDK.
add_subdirectory(${FIREBASE_CPP_SDK_DIR} bin/ EXCLUDE_FROM_ALL)

# # Note that firebase_app needs to be last in the list.
set(firebase_libs firebase_database firebase_auth firebase_app)

if(XCODE)
  if(NOT DEFINED CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET)
    SET (CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET 8.0)
  endif()
endif()

if(NOT DEFINED BUILD_ENGINE_DONE) # to test install_test into root project
  set(COCOS2DX_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cocos2d)
  set(CMAKE_MODULE_PATH ${COCOS2DX_ROOT_PATH}/cmake/Modules/)
  include(CocosBuildSet)
  add_subdirectory(${COCOS2DX_ROOT_PATH}/cocos ${ENGINE_BINARY_PATH}/cocos/core)
endif()

# record sources, headers, resources...
set(GAME_HEADER)
set(GAME_SOURCE)
set(GAME_RES_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/Resources")

 list(APPEND GAME_SOURCE
     Classes/app_delegate.cc
     Classes/tic_tac_toe_layer.cc
     Classes/tic_tac_toe_scene.cc
     Classes/main_menu_scene.cc
     Classes/util.cc
     )

list(APPEND GAME_HEADER
     Classes/app_delegate.h
     Classes/tic_tac_toe_layer.h
     Classes/tic_tac_toe_scene.h
     Classes/main_menu_scene.h
     Classes/util.h
     )   

if(APPLE OR WINDOWS)
  cocos_mark_multi_resources(common_res_files RES_TO "Resources" FOLDERS ${GAME_RES_FOLDER})
endif()

# add cross-platforms source files and header files 
list(APPEND GAME_SOURCE Classes/app_delegate.cc )
list(APPEND GAME_HEADER Classes/app_delegate.h )

if(ANDROID)
  # change APP_NAME to the share library name for Android, it's value depend on AndroidManifest.xml
  set(APP_NAME DemoApp)
  list(APPEND GAME_SOURCE proj.android/app/jni/hellocpp/main.cpp)
elseif(LINUX)
  list(APPEND GAME_SOURCE proj.linux/main.cpp)
elseif(WINDOWS)
  list(APPEND GAME_HEADER proj.win32/main.h proj.win32/resource.h)
  list(APPEND GAME_SOURCE proj.win32/main.cpp proj.win32/game.rc ${common_res_files})
elseif(APPLE)
  if(IOS)
    list(APPEND GAME_HEADER proj.ios_mac/ios/AppController.h 
      proj.ios_mac/ios/RootViewController.h)
    set(APP_UI_RES proj.ios_mac/ios/LaunchScreen.storyboard 
      proj.ios_mac/ios/LaunchScreenBackground.png 
      proj.ios_mac/ios/Images.xcassets)
    list(APPEND GAME_SOURCE proj.ios_mac/ios/main.m proj.ios_mac/ios/AppController.mm
      proj.ios_mac/ios/RootViewController.mm proj.ios_mac/ios/Prefix.pch ${APP_UI_RES})
  elseif(MACOSX)
    set(APP_UI_RES proj.ios_mac/mac/Icon.icns proj.ios_mac/mac/Info.plist)
    list(APPEND GAME_SOURCE proj.ios_mac/mac/main.cpp 
      proj.ios_mac/mac/Prefix.pch ${APP_UI_RES})
  endif()
  list(APPEND GAME_SOURCE ${common_res_files})
endif()

# mark app complie info and libs info
set(all_code_files ${GAME_HEADER} ${GAME_SOURCE})
if(NOT ANDROID)
  add_executable(${APP_NAME} ${all_code_files})
else()
  add_library(${APP_NAME} SHARED ${all_code_files})
  add_subdirectory(${COCOS2DX_ROOT_PATH}/cocos/platform/android 
    ${ENGINE_BINARY_PATH}/cocos/platform)
  target_link_libraries(${APP_NAME} -Wl,--whole-archive cpp_android_spec 
    -Wl,--no-whole-archive)
endif()

target_link_libraries(${APP_NAME} cocos2d "${firebase_libs}" ${ADDITIONAL_LIBS})
target_include_directories(${APP_NAME} PRIVATE Classes 
  PRIVATE ${COCOS2DX_ROOT_PATH}/cocos/audio/include/)

# mark app resources
setup_cocos_app_config(${APP_NAME})
if(APPLE)
  set_target_properties(${APP_NAME} PROPERTIES RESOURCE "${APP_UI_RES}")
  if(MACOSX)
    set_xcode_property(${APP_NAME} INFOPLIST_FILE 
      "${CMAKE_CURRENT_SOURCE_DIR}/proj.ios_mac/mac/Info.plist")
  elseif(IOS)
    set_xcode_property(${APP_NAME} INFOPLIST_FILE 
      "${CMAKE_CURRENT_SOURCE_DIR}/proj.ios_mac/ios/Info.plist")
    set_xcode_property(${APP_NAME} ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon")
  endif()

# For code-signing, set the DEVELOPMENT_TEAM:
#set_xcode_property(${APP_NAME} DEVELOPMENT_TEAM "GRLXXXX2K9")
elseif(WINDOWS)
  cocos_copy_target_dll(${APP_NAME})
endif()

if(LINUX OR WINDOWS)
  cocos_get_resource_path(APP_RES_DIR ${APP_NAME})
  cocos_copy_target_res(${APP_NAME} LINK_TO ${APP_RES_DIR} 
    FOLDERS ${GAME_RES_FOLDER})
endif()

