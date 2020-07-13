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

#include "app_delegate.h"

#include "main_menu_scene.h"
#include "tic_tac_toe_scene.h"

using cocos2d::GLViewImpl;

// Set based on the image width.
const float kFrameWidth = 600;

// Set based on the image height plus 40 for windows bar.
const float kFrameHeight = 640;

AppDelegate::AppDelegate() {}

AppDelegate::~AppDelegate() {}
bool AppDelegate::applicationDidFinishLaunching() {
  auto director = Director::getInstance();
  auto glview = director->getOpenGLView();
  if (glview == NULL) {
    glview = GLViewImpl::create("Tic-Tac-Toe");
    glview->setFrameSize(kFrameWidth, kFrameHeight);
    director->setOpenGLView(glview);
  }

  auto scene = MainMenuScene::createScene();
  director->runWithScene(scene);

  return true;
}

void AppDelegate::applicationDidEnterBackground() {}

void AppDelegate::applicationWillEnterForeground() {}