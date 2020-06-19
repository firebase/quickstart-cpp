#ifndef TICTACTOE_DEMO_CLASSES_TICTACTOELAYER_SCENE_H_
#define TICTACTOE_DEMO_CLASSES_TICTACTOELAYER_SCENE_H_

#include "TicTacToeScene.h"
#include "cocos2d.h"
using cocos2d::Director;
using cocos2d::Event;
using cocos2d::Layer;
using cocos2d::LayerColor;
using cocos2d::Point;
using cocos2d::Sprite;
using cocos2d::Touch;

#include "firebase/app.h"
#include "firebase/auth.h"
#include "firebase/database.h"

using firebase::database::DataSnapshot;
using firebase::database::MutableData;
using firebase::database::TransactionResult;
#include <unordered_set>

#include "firebase/future.h"
#include "firebase/util.h"
using firebase::Future;

static const int kTilesX = 3;
static const int kTilesY = 3;
class SampleValueListener;
class ExpectValueListener;
class TicTacToeLayer : public Layer {
 private:
  typedef TicTacToeLayer self;
  typedef Layer super;

 public:
  TicTacToeLayer(std::string);
  ~TicTacToeLayer();
  virtual void TicTacToeLayer::update(float);
  // Tracks whether the board was unable to build
  bool initialization_failed = false;
  // Tracks the game outcome
  int game_outcome;
  // Creating a string for the join game code and initializing the database
  // reference.
  std::string join_game_uuid;
  /// Firebase Auth, used for logging into Firebase.
  firebase::auth::Auth* auth;

  /// Firebase Realtime Database, the entry point to all database operations.
  firebase::database::Database* database;

  firebase::database::DatabaseReference ref;
  // Creating listeners for database values.
  // The database schema has a top level game_uuid object which includes
  // last_move, total_players and current_player_index fields.
  std::unique_ptr<SampleValueListener> current_player_index_listener;
  std::unique_ptr<SampleValueListener> last_move_listener;
  std::unique_ptr<ExpectValueListener> total_player_listener;
  std::unique_ptr<ExpectValueListener> game_over_listener;
  // Creating lables and a sprites.
  Sprite* board_sprite;
  Sprite* leave_button_sprite;
  cocos2d::Label* game_over_label;
  cocos2d::Label* waiting_label;
  // Creating firebase futures for last_move and current_player_index
  Future<void> future_last_move;
  Future<void> future_current_player_index;
  Future<void> future_game_over;
  // Creating the board, remaining available tile set and player index
  // variables.
  int current_player_index;
  int player_index;
  bool awaiting_opponenet_move;
  int board[kTilesX][kTilesY];
  std::unordered_set<int> remaining_tiles;
  int end_game_frames = 0;
};
#endif  // TICTACTOE_DEMO_CLASSES_TICTACTOELAYER_SCENE_H_
