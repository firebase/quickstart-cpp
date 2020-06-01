#pragma once

#include <array>
#include <cstdlib>

#include "cocos2d.h"

class TicTacToe : public cocos2d::Layer {
 public:
  static cocos2d::Scene* createScene();
  virtual bool init();
  CREATE_FUNC(TicTacToe);
};