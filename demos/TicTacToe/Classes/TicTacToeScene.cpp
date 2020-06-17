#include "TicTacToeScene.h"

// now include our TicTacToeLayer class
#include "MainMenuScene.h"
#include "TicTacToeLayer.h"
#include "cocos2d.h"
using cocos2d::Scene;

Scene* TicTacToe::createScene(std::string game_uuid) {
  // Sets the join_game_uuid to the passed in game_uuid.
  // Builds a simple scene that uses the bottom left cordinate point as (0,0)
  // and can have sprites, labels and layers added onto it.
  Scene* scene = Scene::create();

  // Builds a layer to be placed onto the scene which has access to TouchEvents.
  auto tic_tac_toe_layer = new TicTacToeLayer(game_uuid);
  scene->addChild(tic_tac_toe_layer);

  return scene;
}
