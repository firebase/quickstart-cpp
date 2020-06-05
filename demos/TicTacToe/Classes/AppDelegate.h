#include "cocos2d.h"

class AppDelegate : private cocos2d::Application {
 public:
  AppDelegate();
  ~AppDelegate() override;

  bool applicationDidFinishLaunching() override;
  void applicationDidEnterBackground() override;
  void applicationWillEnterForeground() override;
};