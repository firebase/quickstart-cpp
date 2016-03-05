#ifndef GMA_LOGGING_H
#define GMA_LOGGING_H

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>

const int32_t BUFFER_SIZE = 256;

// Wrap macro in do/while to ensure ;
#define LOG(...) do { \
    char c[BUFFER_SIZE]; \
    snprintf(c, BUFFER_SIZE, __VA_ARGS__); \
    CFStringRef str = CFStringCreateWithCString(kCFAllocatorDefault, c, \
                                                kCFStringEncodingMacRoman); \
    CFShow(str); \
    CFRelease(str); \
  } while (false)

#else
#include <cstddef>
#include <cstdarg>
#include "android/log.h"
#define DEBUG_TAG "FMACPP"
#define LOG(...) \
    ((void)__android_log_print(ANDROID_LOG_INFO, DEBUG_TAG, __VA_ARGS__))

#endif // __APPLE__

#endif // GMA_LOGGING_H
