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

#import <UIKit/UIKit.h>

@interface FBIViewController : UIViewController
@property(nonatomic) NSString* appName;
@end

extern "C" int common_main(int argc, const char* argv[]);

@implementation FBIViewController

- (void)viewDidLoad {
  [super viewDidLoad];
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
    const char* argv[] = {[self.appName UTF8String]};
    common_main(1, argv);
  });
}

@end

static int exit_status = 0;

static UITextView *textView;

// Log a message that can be viewed in "adb logcat".
int LogMessage(const char* format, ...) {
  int rc = 0;
  va_list args;
  NSString *format_string = @(format);
  va_start(args, format);
  NSString* log = [[NSString alloc] initWithFormat:format_string arguments:args];
  va_end(args);
  va_start(args, format);
  NSLogv(format_string, args);
  va_end(args);

  dispatch_async(dispatch_get_main_queue(), ^{
    textView.text = [textView.text stringByAppendingString: @"\n"];
    textView.text = [textView.text stringByAppendingString: log];
  });

  return rc;
}

int main(int argc, char* argv[]) {
  @autoreleasepool {
    UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
  }
  return exit_status;
}

void InitLogWindow(UIViewController* viewController) {
  textView = [[UITextView alloc] initWithFrame:viewController.view.bounds];
  textView.editable = false;
  textView.scrollEnabled = true;
  textView.userInteractionEnabled = true;
  [viewController.view addSubview: textView];
}

@implementation AppDelegate

- (BOOL)application:(UIApplication*)application
    didFinishLaunchingWithOptions:(NSDictionary*)launchOptions {
  // Override point for customization after application launch.
  // Sending invites requires a visible ViewController.
  self.window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
  FBIViewController* viewController = [[FBIViewController alloc] init];
  viewController.appName = [NSString stringWithUTF8String:FIREBASE_TESTAPP_NAME];
  self.window.rootViewController = viewController;
  InitLogWindow(viewController);
  [self.window makeKeyAndVisible];

  return YES;
}

@end
