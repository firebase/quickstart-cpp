#include "TicTacToeScene.h"

#include <array>
#include <cstdlib>

#include "cocos2d.h"

USING_NS_CC;

static const int kTilesX = 3;
static const int kTilesY = 3;
static const int kNumberOfTiles = kTilesX * kTilesY;
static const int kMaxMovesPerPlayer = 1 + kNumberOfTiles / 2;
static const double kScreenWidth = 600;
static const double kScreenHeight = 600;
static const double kTileWidth = (kScreenWidth / kTilesX);
static const double kTileHeight = (kScreenHeight / kTilesY);
static const int kNumberOfPlayers = 2;
static const char* kBoardImageFileName = "tic_tac_toe_board.png";
std::array<const char*, kNumberOfPlayers> kMoveImageFileNames = {
    "tic_tac_toe_x.png", "tic_tac_toe_o.png"};

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
  int current_player_index = 0;
  auto file_names_it = std::begin(kMoveImageFileNames);

  // TODO(grantpostma): This should reflect the size that is set in AppDelegate.
  // Should modify kTileWidth and kTileHeight based on that size auto
  // kScreenWidth = Director::getInstance()->getWinSize().width; auto
  // kScreenHeight =  Director::getInstance()->getWinSize().height;

  auto board_sprite = Sprite::create(kBoardImageFileName);
  board_sprite->setPosition(0, 0);
  board_sprite->setAnchorPoint(Vec2(0.0, 0.0));

  auto touchListener = EventListenerTouchOneByOne::create();
  touchListener->onTouchBegan = [board_sprite, current_player_index](
                                    Touch* touch,
                                    Event* event) mutable -> bool {
    auto bounds = event->getCurrentTarget()->getBoundingBox();

    if (bounds.containsPoint(touch->getLocation())) {
      // This takes the touch location and calculates which tile number [0-8]
      // corresponds to that location
      int move_tile = floor(touch->getLocation().x / kTileWidth) +
                      kTilesX * floor(touch->getLocation().y / kTileHeight);

      auto sprite = Sprite::create(kMoveImageFileNames[current_player_index]);
      // This calculates and sets the position of the sprite based on the
      // move_tile and the constant screen variables.
      sprite->setPosition((.5 + move_tile % kTilesX) * kTileWidth,
                          (.5 + move_tile / kTilesY) * kTileHeight);
      board_sprite->addChild(sprite);
      current_player_index = (current_player_index + 1) % kNumberOfPlayers;
    }
    return true;
  };

  Director::getInstance()
      ->getEventDispatcher()
      ->addEventListenerWithSceneGraphPriority(touchListener, board_sprite);

  this->addChild(board_sprite);

  return true;
}
