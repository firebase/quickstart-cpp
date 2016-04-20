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

static UITextView *textView;

// Log a message that can be viewed in "adb logcat".
int LogMessage(const char* format, ...) {
  int rc = 0;
  va_list args;
  NSString *format_string = [NSString stringWithUTF8String:format];
  va_start(args, format);
  NSString* log = [[NSString alloc] initWithFormat:format_string arguments:args];
  va_end(args);
  va_start(args, format);
  NSLogv(format_string, args);
  va_end(args);

  dispatch_async(dispatch_get_main_queue(), ^{
    textView.text = [textView.text stringByAppendingString:@"\n"];
    textView.text = [textView.text stringByAppendingString: log];
  });

  return rc;
}

bool ProcessEvents(int msec) {
  [NSThread sleepForTimeInterval:static_cast<float>(msec) / 1000.0f];
  return false;
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

  self.window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
  self.window.backgroundColor = [UIColor whiteColor];
  UIViewController *viewController = [[UIViewController alloc] init];
  self.window.rootViewController = viewController;
  [self.window makeKeyAndVisible];

  textView = [[UITextView alloc] initWithFrame:[[UIScreen mainScreen] bounds]];

  textView.editable = false;
  textView.scrollEnabled = true;
  textView.userInteractionEnabled = true;

  [viewController.view addSubview: textView];

  // Override point for customization after application launch.
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0),
                 ^{
                   const char *argv[] = {FIREBASE_TESTAPP_NAME};
                   exit_status = common_main(1, argv);
                 });
  return YES;
}

@end
