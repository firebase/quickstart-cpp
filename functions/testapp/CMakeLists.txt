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

# Platform abstraction layer for the sample.
set(FIREBASE_SAMPLE_DESKTOP_SRCS
  src/desktop/desktop_main.cc
)

# Sample uses some features that require C++ 11, such as lambdas.
set (CMAKE_CXX_STANDARD 11)

# Determine the path to the library based on the platform and configuration.
if(APPLE)
  set(FIREBASE_SDK_LIBDIR ${FIREBASE_CPP_SDK_DIR}/libs/darwin/universal)
  set(ADDITIONAL_LIBS
    pthread
    "-framework CoreFoundation"
    "-framework Security"
  )
elseif(MSVC)
  if(${CMAKE_CL_64})
  set(MSVC_CPU x64)
  else()
  set(MSVC_CPU x86)
  endif()
  if(CMAKE_BUILD_TYPE EQUAL Release)
    set(MSVC_CONFIG Release)
  else()
    set(MSVC_CONFIG Debug)
  endif()
  set(MSVC_VS_VERSION VS2015)
  set(FIREBASE_SDK_LIBDIR
      ${FIREBASE_CPP_SDK_DIR}/libs/windows/${MSVC_VS_VERSION}/${MSVC_RUNTIME_MODE}/${MSVC_CPU}/${MSVC_CONFIG})
  set(ADDITIONAL_LIBS advapi32 ws2_32 crypt32 iphlpapi psapi userenv)
else()
  # The Firebase libraries are not built with glibcxx11, so disable the ABI.
  add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)
  set(LINUX_CPU x86_64)
  set(FIREBASE_SDK_LIBDIR ${FIREBASE_CPP_SDK_DIR}/libs/linux/${LINUX_CPU})
  set(ADDITIONAL_LIBS pthread)
endif()

# Link Firebase libraries.
# NOTE: firebase_app needs to be after all other Firebase libraries.
link_directories(${FIREBASE_SDK_LIBDIR})
set(FIREBASE_LIBS firebase_functions firebase_auth firebase_app)

# Add the Firebase include directory.
set(FIREBASE_SDK_INCLUDEDIR ${FIREBASE_CPP_SDK_DIR}/include)
include_directories(${FIREBASE_SDK_INCLUDEDIR})

# The include directory for the testapp.
include_directories(src)

add_executable(desktop_testapp
  ${FIREBASE_SAMPLE_DESKTOP_SRCS}
  ${FIREBASE_SAMPLE_COMMON_SRCS}
)
target_link_libraries(desktop_testapp
  ${FIREBASE_LIBS}
  ${ADDITIONAL_LIBS}
)

# If a config file is present, copy it into the binary location so that it's
# possible to create the default Firebase app.
set(FOUND_JSON_FILE FALSE)
foreach(config "google-services-desktop.json" "google-services.json")
  if (EXISTS ${config})
    add_custom_command(
      TARGET desktop_testapp POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy
        ${config} $<TARGET_FILE_DIR:desktop_testapp>)
    set(FOUND_JSON_FILE TRUE)
    break()
  endif()
endforeach()
if(NOT FOUND_JSON_FILE)
  message(WARNING "Failed to find either google-services-desktop.json or google-services.json. See the readme.md for more information.")
endif()
