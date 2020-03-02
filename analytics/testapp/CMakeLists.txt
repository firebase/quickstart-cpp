cmake_minimum_required(VERSION 2.8)

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

# Windows runtime mode, either MD or MT depending on whether you are using
# /MD or /MT. For more information see:
# https://msdn.microsoft.com/en-us/library/2kzt1wy3.aspx
set(MSVC_RUNTIME_MODE MD)

project(firebase_testapp)

# Sample source files.
set(FIREBASE_SAMPLE_COMMON_SRCS
  src/main.h
  src/common_main.cc
)

# The include directory for the testapp.
include_directories(src)

# Sample uses some features that require C++ 11, such as lambdas.
set (CMAKE_CXX_STANDARD 11)

if(ANDROID)
  # Build an Android application.

  # Source files used for the Android build.
  set(FIREBASE_SAMPLE_ANDROID_SRCS
    src/android/android_main.cc
  )

  # Build native_app_glue as a static lib
  add_library(native_app_glue STATIC
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

  # Export ANativeActivity_onCreate(),
  # Refer to: https://github.com/android-ndk/ndk/issues/381.
  set(CMAKE_SHARED_LINKER_FLAGS
      "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

  # Define the target as a shared library, as that is what gradle expects.
  set(target_name "android_main")
  add_library(${target_name} SHARED
    ${FIREBASE_SAMPLE_ANDROID_SRCS}
    ${FIREBASE_SAMPLE_COMMON_SRCS}
  )

  target_link_libraries(${target_name}
    log android atomic native_app_glue
  )

  target_include_directories(${target_name} PRIVATE
    ${ANDROID_NDK}/sources/android/native_app_glue)

  set(ADDITIONAL_LIBS)
else()
  # Build a desktop application.

  # Windows runtime mode, either MD or MT depending on whether you are using
  # /MD or /MT. For more information see:
  # https://msdn.microsoft.com/en-us/library/2kzt1wy3.aspx
  set(MSVC_RUNTIME_MODE MD)

  # Platform abstraction layer for the desktop sample.
  set(FIREBASE_SAMPLE_DESKTOP_SRCS
    src/desktop/desktop_main.cc
  )

  set(target_name "desktop_testapp")
  add_executable(${target_name}
    ${FIREBASE_SAMPLE_DESKTOP_SRCS}
    ${FIREBASE_SAMPLE_COMMON_SRCS}
  )

  if(APPLE)
    set(ADDITIONAL_LIBS pthread)
  elseif(MSVC)
    set(ADDITIONAL_LIBS)
  else()
    set(ADDITIONAL_LIBS pthread)
  endif()

  # If a config file is present, copy it into the binary location so that it's
  # possible to create the default Firebase app.
  set(FOUND_JSON_FILE FALSE)
  foreach(config "google-services-desktop.json" "google-services.json")
    if (EXISTS ${config})
      add_custom_command(
        TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
          ${config} $<TARGET_FILE_DIR:${target_name}>)
      set(FOUND_JSON_FILE TRUE)
      break()
    endif()
  endforeach()
  if(NOT FOUND_JSON_FILE)
    message(WARNING "Failed to find either google-services-desktop.json or google-services.json. See the readme.md for more information.")
  endif()
endif()

# Add the Firebase libraries to the target using the function from the SDK.
add_subdirectory(${FIREBASE_CPP_SDK_DIR} bin/ EXCLUDE_FROM_ALL)
# Note that firebase_app needs to be last in the list.
set(firebase_libs firebase_analytics firebase_app)
target_link_libraries(${target_name} "${firebase_libs}" ${ADDITIONAL_LIBS})

