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

#include <stdarg.h>

#include "ios/ios_main.h"
#include "main.h"

@interface FBAViewController : UIViewController

@end

extern "C" int common_main(firebase::admob::AdParent ad_parent);

@implementation FBAViewController

- (void)viewDidLoad {
  [super viewDidLoad];
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
    common_main(self.view);
  });
}

@end

static int exit_status = 0;

// Log a message that can be viewed in the console.
int LogMessage(const char* format, ...) {
  va_list list;
  int rc = 0;
  va_start(list, format);
  NSLogv(@(format), list);
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

- (BOOL)application:(UIApplication*)application
    didFinishLaunchingWithOptions:(NSDictionary*)launchOptions {
  self.window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
  FBAViewController* viewController = [[FBAViewController alloc] init];
  self.window.rootViewController = viewController;
  [self.window makeKeyAndVisible];

  return YES;
}

@end
