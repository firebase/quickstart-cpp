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
std::array<const char*, kNumberOfPlayers> kPlayerTokenFileNames = {
    "tic_tac_toe_x.png", "tic_tac_toe_o.png"};

Scene* TicTacToe::createScene() {
  // Builds a simple scene that uses the bottom left cordinate point as (0,0)
  // and can have sprites, labels and layers added onto it.
  Scene* scene = Scene::create();

  // Builds a layer to be placed onto the scene which has access to TouchEvents.
  TicTacToe* tic_tac_toe_layer = TicTacToe::create();

  scene->addChild(tic_tac_toe_layer);

  return scene;
}

bool TicTacToe::init() {
  if (!Layer::init()) {
    return false;
  }
  int current_player_index = 0;
  auto file_names_it = std::begin(kPlayerTokenFileNames);

  // TODO(grantpostma): This should reflect the size that is set in AppDelegate.
  // (GetVisableSize) Should modify kTileWidth and kTileHeight based on that
  // size. auto kScreenWidth = Director::getInstance()->getWinSize().width; auto
  // kScreenHeight =  Director::getInstance()->getWinSize().height;

  // Creating the board sprite , setting the position to the bottom left of the
  // frame (0,0), and finally moving the anchor point from the center of the
  // image(default) to the bottom left, Vec2(0.0,0.0).
  Sprite* board_sprite = Sprite::create(kBoardImageFileName);
  if (!board_sprite) {
    log("kBoardImageFileName: %s file not found.", kBoardImageFileName);
    exit(true);
  }
  board_sprite->setPosition(0, 0);
  board_sprite->setAnchorPoint(Vec2(0.0, 0.0));

  // Adding a function to determine which tile was selected to the onTouchBegan
  // listener.
  auto touch_listener = EventListenerTouchOneByOne::create();
  touch_listener->onTouchBegan = [board_sprite, current_player_index](
                                     Touch* touch,
                                     Event* event) mutable -> bool {
    auto bounds = event->getCurrentTarget()->getBoundingBox();

    if (bounds.containsPoint(touch->getLocation())) {
      // Calculates the tile number [0-8] which corresponds to the touch
      // location.
      int selected_tile = floor(touch->getLocation().x / kTileWidth) +
                          kTilesX * floor(touch->getLocation().y / kTileHeight);

      auto sprite = Sprite::create(kPlayerTokenFileNames[current_player_index]);
      if (sprite == NULL) {
        log("kPlayerTokenFileNames: %s file not found.",
            kPlayerTokenFileNames[current_player_index]);
        exit(true);
      }
      // Calculate and set the position of the sprite based on the
      // move_tile and the constant screen variables.
      sprite->setPosition((.5 + selected_tile % kTilesX) * kTileWidth,
                          (.5 + selected_tile / kTilesY) * kTileHeight);
      board_sprite->addChild(sprite);
      current_player_index = (current_player_index + 1) % kNumberOfPlayers;
    }
    return true;
  };

  Director::getInstance()
      ->getEventDispatcher()
      ->addEventListenerWithSceneGraphPriority(touch_listener, board_sprite);

  this->addChild(board_sprite);

  return true;
}
