#include "TicTacToeScene.h"

using cocos2d::Scene;

Scene* TicTacToe::createScene(const std::string& game_uuid,
                              firebase::database::Database* main_menu_database,
                              const std::string& main_menu_user_uid) {
  // Sets the join_game_uuid to the passed in game_uuid.
  // Builds a simple scene that uses the bottom left cordinate point as (0,0)
  // and can have sprites, labels and layers added onto it.
  Scene* scene = Scene::create();

  // Builds a layer to be placed onto the scene which has access to TouchEvents.
  // This TicTacToe layer being created is owned by scene.
  auto tic_tac_toe_layer =
      new TicTacToeLayer(game_uuid, main_menu_database, main_menu_user_uid);
  scene->addChild(tic_tac_toe_layer);

  return scene;
}
