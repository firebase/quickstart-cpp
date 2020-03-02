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

#import <UIKit/UIKit.h>

#include <pthread.h>
#include <unistd.h>

#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include "main.h"

extern "C" int common_main(int argc, const char* argv[]);

@interface AppDelegate : UIResponder<UIApplicationDelegate>

@property(nonatomic, strong) UIWindow *window;

@end

@interface FTAViewController : UIViewController

@end

static int g_exit_status = 0;
static bool g_shutdown = false;
static NSCondition *g_shutdown_complete;
static NSCondition *g_shutdown_signal;
static UITextView *g_text_view;
static UIView *g_parent_view;

@implementation FTAViewController

- (void)viewDidLoad {
  [super viewDidLoad];
  g_parent_view = self.view;
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
    const char *argv[] = {TESTAPP_NAME};
    [g_shutdown_signal lock];
    g_exit_status = common_main(1, argv);
    [g_shutdown_complete signal];
  });
}

@end
namespace app_framework {

bool ProcessEvents(int msec) {
  [g_shutdown_signal
      waitUntilDate:[NSDate dateWithTimeIntervalSinceNow:static_cast<float>(msec) / 1000.0f]];
  return g_shutdown;
}

std::string PathForResource() {
  NSArray<NSString *> *paths =
      NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
  NSString *documentsDirectory = paths.firstObject;
  // Force a trailing slash by removing any that exists, then appending another.
  return std::string(
      [[documentsDirectory stringByStandardizingPath] stringByAppendingString:@"/"].UTF8String);
}

WindowContext GetWindowContext() {
  return g_parent_view;
}

void LogMessage(const char *format, ...) {
  va_list list;
  va_start(list, format);
  LogMessageV(format, list);
  va_end(list);
}

// Log a message that can be viewed in the console.
void LogMessageV(const char *format, va_list list) {
  NSString *formatString = @(format);

  NSString *message = [[NSString alloc] initWithFormat:formatString arguments:list];

  NSLog(@"%@", message);
  message = [message stringByAppendingString:@"\n"];

  fputs(message.UTF8String, stdout);
  fflush(stdout);
}

// Log a message that can be viewed in the console.
void AddToTextView(const char *str) {
  NSString *message = @(str);

  dispatch_async(dispatch_get_main_queue(), ^{
    g_text_view.text = [g_text_view.text stringByAppendingString:message];
    NSRange range = NSMakeRange(g_text_view.text.length, 0);
    [g_text_view scrollRangeToVisible:range];
  });
}

// Remove all lines starting with these strings.
static const char *const filter_lines[] = {nullptr};

bool should_filter(const char *str) {
  for (int i = 0; filter_lines[i] != nullptr; ++i) {
    if (strncmp(str, filter_lines[i], strlen(filter_lines[i])) == 0) return true;
  }
  return false;
}

void *stdout_logger(void *filedes_ptr) {
  int fd = reinterpret_cast<int *>(filedes_ptr)[0];
  std::string buffer;
  char bufchar;
  while (int n = read(fd, &bufchar, 1)) {
    if (bufchar == '\0') {
      break;
    }
    buffer = buffer + bufchar;
    if (bufchar == '\n') {
      if (!should_filter(buffer.c_str())) {
        app_framework::AddToTextView(buffer.c_str());
      }
      buffer.clear();
    }
  }
  return nullptr;
}

void RunOnBackgroundThread(void* (*func)(void*), void* data) {
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
      func(data);
    });
}

}  // namespace app_framework

int main(int argc, char* argv[]) {
  // Pipe stdout to call LogToTextView so we can see the gtest output.
  int filedes[2];
  assert(pipe(filedes) != -1);
  assert(dup2(filedes[1], STDOUT_FILENO) != -1);
  pthread_t thread;
  pthread_create(&thread, nullptr, app_framework::stdout_logger, reinterpret_cast<void *>(filedes));
  @autoreleasepool {
    UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
  }
  // Signal to stdout_logger to exit.
  write(filedes[1], "\0", 1);
  pthread_join(thread, nullptr);
  close(filedes[0]);
  close(filedes[1]);

  NSLog(@"Application Exit");
  return g_exit_status;
}

@implementation AppDelegate

- (BOOL)application:(UIApplication*)application
    didFinishLaunchingWithOptions:(NSDictionary*)launchOptions {
  g_shutdown_complete = [[NSCondition alloc] init];
  g_shutdown_signal = [[NSCondition alloc] init];
  [g_shutdown_complete lock];

  self.window = [[UIWindow alloc] initWithFrame:[UIScreen mainScreen].bounds];
  FTAViewController *viewController = [[FTAViewController alloc] init];
  self.window.rootViewController = viewController;
  [self.window makeKeyAndVisible];

  g_text_view = [[UITextView alloc] initWithFrame:viewController.view.bounds];

  g_text_view.accessibilityIdentifier = @"Logger";
  g_text_view.editable = NO;
  g_text_view.scrollEnabled = YES;
  g_text_view.userInteractionEnabled = YES;
  g_text_view.font = [UIFont fontWithName:@"Courier" size:10];
  [viewController.view addSubview:g_text_view];

  return YES;
}

- (void)applicationWillTerminate:(UIApplication *)application {
  g_shutdown = true;
  [g_shutdown_signal signal];
  [g_shutdown_complete wait];
}
@end
