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

#ifndef TICTACTOE_DEMO_CLASSES_TICTACTOE_SCENE_H_
#define TICTACTOE_DEMO_CLASSES_TICTACTOE_SCENE_H_

#include "cocos2d.h"
#include "main_menu_scene.h"
#include "tic_tac_toe_layer.h"
#include "util.h"

class TicTacToe : public cocos2d::Layer {
 public:
  // Builds a simple scene that uses the bottom left cordinate point as (0,0)
  // and can have sprites, labels and nodes added onto it.
  static cocos2d::Scene* createScene(const std::string&,
                                     firebase::database::Database*,
                                     const std::string&);

  // Defines a create type for a specific type, in this case a Layer.
  CREATE_FUNC(TicTacToe);
};
#endif  // TICTACTOE_DEMO_CLASSES_TICTACTOE_SCENE_H_