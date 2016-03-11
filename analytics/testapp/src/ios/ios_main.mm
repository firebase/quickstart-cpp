// Copyright 2016 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#include <stdarg.h>

#include "ios/ios_main.h"
#include "main.h"

extern "C" int common_main(int argc, const char* argv[]);

static int exit_status = 0;

// Log a message that can be viewed in "adb logcat".
int LogMessage(const char* format, ...) {
  va_list list;
  int rc = 0;
  va_start(list, format);
  NSLogv([NSString stringWithUTF8String:format], list);
  va_end(list);
  return rc;
}

int main(int argc, char* argv[]) {
  @autoreleasepool {
    UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
  }
  return exit_status;
}

@implementation AppDelegate

- (BOOL)application:(UIApplication *)application
    didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
  // Override point for customization after application launch.
  dispatch_async(dispatch_get_main_queue(), ^{
    const char *argv[] = {FIREBASE_TESTAPP_NAME};
    exit_status = common_main(1, argv);
  });
  return YES;
}

@end
