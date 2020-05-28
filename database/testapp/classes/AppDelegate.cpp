#include "AppDelegate.h"

#include "TicTacToeScene.h"

USING_NS_CC;

// The size constants are defined as floats because that is what is expected in
// setFrameSize().
static const float kFrameWidth = 600;
static const float kFrameHeight = 600;

AppDelegate::AppDelegate() {}

AppDelegate::~AppDelegate() {}

bool AppDelegate::applicationDidFinishLaunching() {
  auto director = Director::getInstance();
  auto glview = director->getOpenGLView();
  if (!glview) {
    glview = GLViewImpl::create("Hello World");
    glview->setFrameSize(kFrameWidth, kFrameHeight);
    director->setOpenGLView(glview);
  }

  auto scene = TicTacToe::createScene();
  director->runWithScene(scene);

  return true;
}

void AppDelegate::applicationDidEnterBackground() {}

void AppDelegate::applicationWillEnterForeground() {}