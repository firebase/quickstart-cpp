#include "TicTacToeScene.h"

USING_NS_CC;

Scene* TicTacToe::createScene() {
  auto scene = Scene::create();
  auto layer = TicTacToe::create();

  scene->addChild(layer);

  return scene;
}

bool TicTacToe::init() {
  if (!Layer::init()) {
    return false;
  }

  const int kScreenWidth = Director::getInstance()->getVisibleSize().width;
  const int kScreenHeight = Director::getInstance()->getVisibleSize().height;

  auto board_sprite = Sprite::create("tic_tac_toe_board.png");
  auto x_mark_sprite = Sprite::create("tic_tac_toe_x.png");

  board_sprite->setPosition(0, 0);
  board_sprite->setAnchorPoint(Vec2(0.0, 0.0));
  board_sprite->setScale(kScreenWidth /
                         board_sprite->getBoundingBox().size.width);

  x_mark_sprite->setPosition(0, 0);
  x_mark_sprite->setAnchorPoint(Vec2(0.5, 0.5));
  board_sprite->addChild(x_mark_sprite);

  this->addChild(board_sprite);

  return true;
}
