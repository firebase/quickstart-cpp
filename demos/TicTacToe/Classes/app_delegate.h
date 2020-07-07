// Copyright 2020 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef TICTACTOE_DEMO_CLASSES_APPDELEGATE_SCENE_H_
#define TICTACTOE_DEMO_CLASSES_APPDELEGATE_SCENE_H_
#include "cocos2d.h"

class AppDelegate : private cocos2d::Application {
 public:
  AppDelegate();
  ~AppDelegate() override;

  bool applicationDidFinishLaunching() override;
  void applicationDidEnterBackground() override;
  void applicationWillEnterForeground() override;
};
#endif  // TICTACTOE_DEMO_CLASSES_APPDELEGATE_SCENE_H_