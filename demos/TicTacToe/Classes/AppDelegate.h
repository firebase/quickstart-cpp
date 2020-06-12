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