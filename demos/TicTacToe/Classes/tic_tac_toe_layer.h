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

#include <string>
#include <unordered_set>

#include "cocos2d.h"
#include "firebase/database.h"
#include "firebase/future.h"

using cocos2d::Label;
using cocos2d::Layer;
using cocos2d::Sprite;
using firebase::Future;
using firebase::database::Database;
using std::string;
using std::unique_ptr;

// Tile Constants.
static const int kTilesX = 3;
static const int kTilesY = 3;

// Firebase listeners.
class SampleValueListener;
class ExpectValueListener;

class TicTacToeLayer : public Layer {
 public:
  // Derived from Layer class with input paramters for the game_uid, database
  // and user_uid and overrides Layer::update().
  TicTacToeLayer(string, Database*, string);
  ~TicTacToeLayer();

 private:
  // The game loop for this layer which runs every frame once scheduled using
  // this->scheduleUpdate(). It constantly checks current_player_index_listener_
  // and game_over_listener so it can take action accordingly.
  void update(float) override;

  // Initializes the game board cocos2d componenets and game variables.
  void InitializeBoard();

  // Initializes the player data, based on the join_game_uid.
  void InitializePlayerData();

  // Shows the end game message before returning back to the previous scene.
  void EndGame();

  // Initializes the value listeners for current_player_, last_move_, game_over_
  // and total_players_.
  void InitializeDatabaseListeners();

  // Updates the board based on the move taken from the game instance in
  // the database.
  void UpdateBoard();

  // Tracks whether the board was unable to build.
  bool initialization_failed_ = false;

  // Tracks the game outcome.
  int game_outcome_;

  // String for the join game code and initialize the database
  // reference.
  string join_game_uid_;

  // User uid to update the user's record after the game is over.
  string user_uid_;

  // Firebase Realtime Database, the entry point to all database operations.
  //
  // The database schema has a top level game_uid object which includes
  // last_move, total_players and current_player_index_ fields.
  Database* database_;
  firebase::database::DatabaseReference ref_;

  // Listeners for database values.
  unique_ptr<SampleValueListener> current_player_index_listener_;
  unique_ptr<SampleValueListener> last_move_listener_;
  unique_ptr<ExpectValueListener> total_player_listener_;
  unique_ptr<ExpectValueListener> game_over_listener_;

  // Lables and a sprites.
  Sprite* board_sprite_;
  Sprite* leave_button_sprite_;
  Label* game_over_label_;
  Label* waiting_label_;

  // Firebase futures for last_move and current_player_index_.
  Future<void> future_last_move_;
  Future<void> future_current_player_index_;
  Future<void> future_game_over_;
  Future<void> future_create_game_;

  int current_player_index_;
  int player_index_;
  int board[kTilesX][kTilesY];

  bool awaiting_opponenet_move_;

  // Unordered set of remaining tiles available for player moves.
  std::unordered_set<int> remaining_tiles_;

  // Amount of frames the screen has been in the end game state.
  // Number of seconds x 60.
  int end_game_frames_ = 120;
};
#endif  // TICTACTOE_DEMO_CLASSES_TICTACTOELAYER_SCENE_H_