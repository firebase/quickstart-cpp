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

#ifndef TICTACTOE_DEMO_CLASSES_TICTACTOELAYER_SCENE_H_
#define TICTACTOE_DEMO_CLASSES_TICTACTOELAYER_SCENE_H_

#include <algorithm>
#include <array>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>

#include "MainMenuScene.h"
#include "TicTacToeScene.h"
#include "cocos2d.h"

using cocos2d::Director;
using cocos2d::Event;
using cocos2d::Layer;
using cocos2d::LayerColor;
using cocos2d::Point;
using cocos2d::Sprite;
using cocos2d::Touch;
using firebase::Future;
using firebase::database::DataSnapshot;
using firebase::database::MutableData;
using firebase::database::TransactionResult;
using std::string;

static const int kTilesX = 3;
static const int kTilesY = 3;
class SampleValueListener;
class ExpectValueListener;
class TicTacToeLayer : public Layer {
 private:
  typedef TicTacToeLayer self;
  typedef Layer super;

 public:
  TicTacToeLayer(std::string, firebase::database::Database*, std::string);
  ~TicTacToeLayer();
  void TicTacToeLayer::update(float) override;

  // Tracks whether the board was unable to build.
  bool initialization_failed = false;

  // Tracks the game outcome.
  int game_outcome;

  // Create a string for the join game code and initialize the database
  // reference.
  std::string join_game_uuid;

  // User uid to update the user's record after the game is over.
  std::string user_uid;

  /// Firebase Realtime Database, the entry point to all database operations.
  firebase::database::Database* database;
  firebase::database::DatabaseReference ref;

  // Create listeners for database values.
  // The database schema has a top level game_uuid object which includes
  // last_move, total_players and current_player_index fields.
  std::unique_ptr<SampleValueListener> current_player_index_listener;
  std::unique_ptr<SampleValueListener> last_move_listener;
  std::unique_ptr<ExpectValueListener> total_player_listener;
  std::unique_ptr<ExpectValueListener> game_over_listener;

  // Create lables and a sprites.
  Sprite* board_sprite;
  Sprite* leave_button_sprite;
  cocos2d::Label* game_over_label;
  cocos2d::Label* waiting_label;

  // Create firebase futures for last_move and current_player_index
  Future<void> future_last_move;
  Future<void> future_current_player_index;
  Future<void> future_game_over;

  // Create the board, remain available tile set and player index
  // variables.
  int current_player_index;
  int player_index;
  bool awaiting_opponenet_move;
  int board[kTilesX][kTilesY];
  std::unordered_set<int> remaining_tiles;
  int end_game_frames = 0;
};
#endif  // TICTACTOE_DEMO_CLASSES_TICTACTOELAYER_SCENE_H_
