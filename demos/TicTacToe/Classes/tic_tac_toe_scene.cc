// Copyright 2020 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "tic_tac_toe_scene.h"

#include <string>

#include "cocos2d.h"
#include "firebase/database.h"
#include "tic_tac_toe_layer.h"

using cocos2d::Scene;

Scene* TicTacToe::createScene(const std::string& game_uuid,
                              firebase::database::Database* main_menu_database,
                              const std::string& main_menu_user_uid) {
  // Sets the join_game_uuid to the passed in game_uuid.
  // Builds a simple scene that uses the bottom left cordinate point as (0,0)
  // and can have sprites, labels and layers added onto it.
  Scene* scene = Scene::create();

  // Builds a layer to be placed onto the scene which has access to TouchEvents.
  // This TicTacToe layer created is owned by the scene.
  auto tic_tac_toe_layer =
      new TicTacToeLayer(game_uuid, main_menu_database, main_menu_user_uid);
  scene->addChild(tic_tac_toe_layer);

  return scene;
}
