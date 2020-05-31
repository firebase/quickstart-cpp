#include "AppDelegate.h"

#include "TicTacToeScene.h"

USING_NS_CC;

extern const float kFrameWidth = 600;
extern const float kFrameHeight = 600;

AppDelegate::AppDelegate() {}

AppDelegate::~AppDelegate() {}

bool AppDelegate::applicationDidFinishLaunching() {
  auto director = Director::getInstance();
  auto glview = director->getOpenGLView();
  if (!glview) {
    glview = GLViewImpl::create("Tic-Tac-Toe");
    glview->setFrameSize(kFrameWidth, kFrameHeight);
    director->setOpenGLView(glview);
  }

  auto scene = TicTacToe::createScene();
  director->runWithScene(scene);

  return true;
}

void AppDelegate::applicationDidEnterBackground() {}

void AppDelegate::applicationWillEnterForeground() {}