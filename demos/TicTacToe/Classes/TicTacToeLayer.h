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
 public:
  TicTacToeLayer(std::string, firebase::database::Database*, std::string);
  ~TicTacToeLayer();

 private:
  typedef TicTacToeLayer self;
  typedef Layer super;
  void TicTacToeLayer::update(float) override;

  // Tracks whether the board was unable to build.
  bool initialization_failed_ = false;

  // Tracks the game outcome.
  int game_outcome_;

  // Create a string for the join game code and initialize the database
  // reference.
  std::string join_game_uuid_;

  // User uid to update the user's record after the game is over.
  std::string user_uid_;

  /// Firebase Realtime Database, the entry point to all database operations.
  firebase::database::Database* database_;
  firebase::database::DatabaseReference ref_;

  // Create listeners for database values.
  // The database schema has a top level game_uuid object which includes
  // last_move, total_players and current_player_index_ fields.
  std::unique_ptr<SampleValueListener> current_player_index_listener_;
  std::unique_ptr<SampleValueListener> last_move_listener_;
  std::unique_ptr<ExpectValueListener> total_player_listener_;
  std::unique_ptr<ExpectValueListener> game_over_listener_;

  // Create lables and a sprites.
  Sprite* board_sprite_;
  Sprite* leave_button_sprite_;
  cocos2d::Label* game_over_label_;
  cocos2d::Label* waiting_label_;

  // Create firebase futures for last_move and current_player_index_
  Future<void> future_last_move_;
  Future<void> future_current_player_index_;
  Future<void> future_game_over_;

  // Create the board, remain available tile set and player index
  // variables.
  int current_player_index_;
  int player_index_;
  bool awaiting_opponenet_move_;
  int board[kTilesX][kTilesY];
  std::unordered_set<int> remaining_tiles_;
  int end_game_frames_ = 0;
};
#endif  // TICTACTOE_DEMO_CLASSES_TICTACTOELAYER_SCENE_H_
