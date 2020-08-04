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

#include "tic_tac_toe_layer.h"

#include <algorithm>
#include <array>
#include <string>
#include <unordered_set>
#include <vector>

#include "cocos/ui/UIButton.h"
#include "cocos/ui/UIWidget.h"
#include "cocos2d.h"
#include "firebase/database.h"
#include "firebase/variant.h"
#include "util.h"

using cocos2d::CallFunc;
using cocos2d::Color4B;
using cocos2d::DelayTime;
using cocos2d::Director;
using cocos2d::Event;
using cocos2d::EventListenerTouchOneByOne;
using cocos2d::Label;
using cocos2d::Sequence;
using cocos2d::Sprite;
using cocos2d::Touch;
using cocos2d::Vec2;
using cocos2d::ui::Button;
using cocos2d::ui::Widget;
using firebase::Variant;
using firebase::database::DataSnapshot;
using firebase::database::Error;
using firebase::database::TransactionResult;
using firebase::database::ValueListener;
using std::array;
using std::make_unique;
using std::string;
using std::vector;

// Player constants.
static const int kEmptyTile = -1;
static const int kPlayerOne = 0;
static const int kPlayerTwo = 1;
static const int kNumberOfPlayers = 2;

// End game outcomes.
static const enum kGameOutcome {
  kGameWon = 0,
  kGameLost,
  kGameTied,
  kGameDisbanded
};

// States for button images.
static const enum kImageState { kNormal, kPressed };

// create an array that has indicies of enum kGameOutcome and maps that to the
// database outcome key.
static const const char* kGameOutcomeStrings[] = {"wins", "loses", "ties",
                                                  "disbanded"};
// Game board dimensions.
extern const int kTilesX;
extern const int kTilesY;
static const int kNumberOfTiles = kTilesX * kTilesY;

// Game board dimensions.
static const double kBoardWidth = 487;
static const double kBoardHeight = 484;
static const double kBoardLineWidth = 23;
static const double kBoardLineHeight = 26;
static const double kTileWidthHitBox = (kBoardWidth / kTilesX);
static const double kTileHeightHitBox = (kBoardHeight / kTilesY);
static const double kTileWidth =
    ((kBoardWidth - ((kTilesX - 1) * kBoardLineWidth)) / kTilesX);
static const double kTileHeight =
    ((kBoardHeight - ((kTilesY - 1) * kBoardLineHeight)) / kTilesY);
static const Vec2 kBoardOrigin =
    Vec2(300 - (kBoardWidth / 2), 320 - (kBoardHeight / 2));

// The screen will display the end game text for 2 seconds (120frames/60fps).
static const int kEndGameFramesMax = 120;

// Constants for the image filenames.
static const char* kTextFieldImage = "text_field_3.png";
static const char* kBoardImageFileName = "tic_tac_toe_board.png";
static const array<char*, 2> kLeaveButton = {"leave_button.png",
                                             "leave_button_pressed.png"};
static const array<const char*, /*number_of_outcomes*/ 4> kGameOutcomeImages = {
    "outcome_won.png", "outcome_lost.png", "outcome_tied.png",
    "outcome_disbanded.png"};
static const array<const char*, kNumberOfPlayers> kPlayerTokenFileNames = {
    "tic_tac_toe_x.png", "tic_tac_toe_o.png"};

// An example of a ValueListener object. This specific version will
// simply log every value it sees, and store them in a list so we can
// confirm that all values were received.
class SampleValueListener : public ValueListener {
 public:
  void OnValueChanged(const DataSnapshot& snapshot) override {
    LogMessage("  ValueListener.OnValueChanged(%s)",
               snapshot.value().AsString().string_value());
    last_seen_value_ = snapshot.value();
    seen_values_.push_back(snapshot.value());
  }
  void OnCancelled(const Error& error_code,
                   const char* error_message) override {
    LogMessage("ERROR: SampleValueListener canceled: %d: %s", error_code,
               error_message);
  }
  const Variant& last_seen_value() { return last_seen_value_; }
  bool seen_value(const Variant& value) {
    return std::find(seen_values_.begin(), seen_values_.end(), value) !=
           seen_values_.end();
  }
  size_t num_seen_values() { return seen_values_.size(); }

 private:
  Variant last_seen_value_;
  vector<Variant> seen_values_;
};

// An example ChildListener class.
class SampleChildListener : public firebase::database::ChildListener {
 public:
  void OnChildAdded(const DataSnapshot& snapshot,
                    const char* previous_sibling) override {
    LogMessage("  ChildListener.OnChildAdded(%s)", snapshot.key());
    events_.push_back(string("added ") + snapshot.key());
  }
  void OnChildChanged(const DataSnapshot& snapshot,
                      const char* previous_sibling) override {
    LogMessage("  ChildListener.OnChildChanged(%s)", snapshot.key());
    events_.push_back(string("changed ") + snapshot.key());
  }
  void OnChildMoved(const DataSnapshot& snapshot,
                    const char* previous_sibling) override {
    LogMessage("  ChildListener.OnChildMoved(%s)", snapshot.key());
    events_.push_back(string("moved ") + snapshot.key());
  }
  void OnChildRemoved(const DataSnapshot& snapshot) override {
    LogMessage("  ChildListener.OnChildRemoved(%s)", snapshot.key());
    events_.push_back(string("removed ") + snapshot.key());
  }
  void OnCancelled(const Error& error_code,
                   const char* error_message) override {
    LogMessage("ERROR: SampleChildListener canceled: %d: %s", error_code,
               error_message);
  }

  // Get the total number of Child events this listener saw.
  size_t total_events() { return events_.size(); }

  // Get the number of times this event was seen.
  int num_events(const string& event) {
    int count = 0;
    for (int i = 0; i < events_.size(); i++) {
      if (events_[i] == event) {
        count++;
      }
    }
    return count;
  }

 public:
  // Vector of strings that contains the events in the order in which they
  // occurred.
  vector<string> events_;
};

// A ValueListener that expects a specific value to be set.
class ExpectValueListener : public ValueListener {
 public:
  explicit ExpectValueListener(Variant wait_value)
      : wait_value_(wait_value.AsString()), got_value_(false) {}
  void OnValueChanged(const DataSnapshot& snapshot) override {
    if (snapshot.value().AsString() == wait_value_) {
      got_value_ = true;
    } else {
      LogMessage(
          "FAILURE: ExpectValueListener did not receive the expected result.");
    }
  }
  void OnCancelled(const Error& error_code,
                   const char* error_message) override {
    LogMessage("ERROR: ExpectValueListener canceled: %d: %s", error_code,
               error_message);
  }

  bool got_value() { return got_value_; }

 private:
  Variant wait_value_;
  bool got_value_;
};

// A function that returns true if any of the row
// is crossed with the same player's move
static bool RowCrossed(int board[][kTilesY]) {
  for (int i = 0; i < kTilesY; i++) {
    if (board[i][0] == board[i][1] && board[i][1] == board[i][2] &&
        board[i][0] != kEmptyTile)
      return true;
  }
  return false;
}

// A function that returns true if any of the column
// is crossed with the same player's move
static bool ColumnCrossed(int board[][kTilesY]) {
  for (int i = 0; i < kTilesX; i++) {
    if (board[0][i] == board[1][i] && board[1][i] == board[2][i] &&
        board[0][i] != kEmptyTile)
      return (true);
  }
  return (false);
}

// A function that returns true if any of the diagonal
// is crossed with the same player's move
static bool DiagonalCrossed(int board[][kTilesY]) {
  if (board[0][0] == board[1][1] && board[1][1] == board[2][2] &&
      board[0][0] != kEmptyTile)
    return (true);

  if (board[0][2] == board[1][1] && board[1][1] == board[2][0] &&
      board[0][2] != kEmptyTile)
    return (true);

  return (false);
}

// A function that returns true if the game is over
// else it returns a false
static bool GameOver(int board[][kTilesY]) {
  return (RowCrossed(board) || ColumnCrossed(board) || DiagonalCrossed(board));
}

// Creates all of the layer's components, joins game based on game_uid existing
// in the database.
TicTacToeLayer::TicTacToeLayer(string game_uid,
                               firebase::database::Database* main_menu_database,
                               string main_menu_user) {
  // Initializes starting game variables.
  join_game_uid_ = game_uid;
  current_player_index_ = kPlayerOne;
  game_outcome_ = kGameWon;
  database_ = main_menu_database;
  user_uid_ = main_menu_user;

  // Sets the initial values for the player based on join_game_uid's existence.
  this->InitializePlayerData();

  // Initializes the board and cocos2d board components.
  this->InitializeBoard();

  // Initializes the SampleValue and ExpectValue listeners.
  this->InitializeDatabaseListeners();

  // Schedules the update method for this scene.
  this->scheduleUpdate();
}

// Called automatically every frame. The update is scheduled at the end of
// the constructor.
void TicTacToeLayer::update(float /*delta*/) {
  // Pops the scene if the initialization fails.
  if (initialization_failed_) {
    Director::getInstance()->popScene();
  }
  // Performs the actions of the other player when the
  // current_player_index_listener_ is equal to the player index.
  else if (current_player_index_listener_->last_seen_value() == player_index_ &&
           awaiting_opponenet_move_ == true) {
    this->UpdateBoard();
  }
  // Shows the end game label for kEndGameFramesMax to show the result of the
  // game.
  else if (game_over_listener_->got_value() && !displaying_outcome_) {
    this->DisplayGameOutcome();
  }
  // Updates the waiting label to show its your move.
  else if (total_player_listener_->got_value() &&
           awaiting_opponenet_move_ == false) {
    waiting_label_->setString("Your Move");
  }
}
// Creates all of the cocos2d componets and places them on the layer.
// Initializes game board.
void TicTacToeLayer::InitializeBoard() {
  // Creates the layer background color.
  const auto background =
      cocos2d::LayerColor::create(Color4B(255, 255, 255, 255));
  this->addChild(background);

  // Creates the game board.
  board_sprite_ = Sprite::create(kBoardImageFileName);
  board_sprite_->setPosition(Vec2(300, 300));

  // Creates the leave button.
  leave_button_ = Button::create(kLeaveButton[kNormal], kLeaveButton[kPressed]);
  leave_button_->setPosition(Vec2(100, 575));
  this->addChild(leave_button_, /*layer_index=*/1);

  leave_button_->addTouchEventListener(
      [this](Ref* sender, Widget::TouchEventType type) {
        switch (type) {
          case Widget::TouchEventType::ENDED:
            // Update the game_outcome_ to reflect if the you rage quit or
            // left pre-match.
            if (remaining_tiles_.size() == kNumberOfTiles) {
              game_outcome_ = kGameDisbanded;
            } else {
              game_outcome_ = kGameLost;
            }

            WaitForCompletion(ref_.Child("game_over").SetValue(true),
                              "setGameOver");
            break;
          default:
            break;
        }
      });

  // Creates the label for the game uid.
  const auto game_uid_position = Vec2(500, 575);
  Label* game_uid_label =
      Label::createWithTTF(join_game_uid_, "fonts/GoogleSans-Regular.ttf", 30);
  game_uid_label->setTextColor(Color4B(0, 0, 0, 100));
  game_uid_label->setPosition(game_uid_position);
  this->addChild(game_uid_label, /*layer_index=*/1);

  // Creates the text box background for the game uid label.
  const auto game_uid_background = Sprite::create(kTextFieldImage);
  game_uid_background->setPosition(game_uid_position);
  this->addChild(game_uid_background, /*layer_index=*/0);

  // Creates the label that displays "Waiting" or "Your Move" depending on
  // current_player_.
  waiting_label_ =
      Label::createWithTTF("Waiting", "fonts/GoogleSans-Regular.ttf", 30);
  waiting_label_->setTextColor(Color4B(255, 82, 82, 240));
  waiting_label_->setPosition(Vec2(300, 575));
  this->addChild(waiting_label_, /*layer_index=*/1);

  // Set up a 3*3 Tic-Tac-Toe board for tracking results.
  for (int i = 0; i < kTilesY; i++) {
    for (int j = 0; j < kTilesX; j++) {
      board[i][j] = kEmptyTile;
      remaining_tiles_.insert((i * kTilesX) + j);
    };
  }

  // Add a function to determine which tile was selected to the onTouchBegan
  // listener.
  auto board_touch_listener = EventListenerTouchOneByOne::create();
  board_touch_listener->onTouchBegan = [this](Touch* touch,
                                              Event* event) mutable -> bool {
    if (!total_player_listener_->got_value()) return true;
    if (current_player_index_listener_->last_seen_value() != player_index_)
      return true;

    const auto bounds = event->getCurrentTarget()->getBoundingBox();

    // Check to make sure the touch location is within the bounds of the
    // board.
    if (bounds.containsPoint(touch->getLocation())) {
      // Calculates the tile number [0-8] which corresponds to the touch
      // location.
      const auto touch_board_location =
          Vec2(touch->getLocation().x - kBoardOrigin.x,
               touch->getLocation().y - kBoardOrigin.y);
      int selected_tile =
          floor(touch_board_location.x / kTileWidthHitBox) +
          kTilesX * floor(touch_board_location.y / kTileHeightHitBox);
      if (remaining_tiles_.find(selected_tile) == remaining_tiles_.end())
        return true;

      auto sprite =
          Sprite::create(kPlayerTokenFileNames[current_player_index_]);
      if (sprite == NULL) {
        CCLOG("kPlayerTokenFileNames: %s file not found.",
              kPlayerTokenFileNames[current_player_index_]);
        exit(true);
      }

      // Calculates and sets the position of the sprite based on the
      // move_tile and the constant screen variables.
      const auto x_tile = selected_tile % kTilesX;
      const auto y_tile = selected_tile / kTilesY;
      sprite->setPosition(
          (.5 + x_tile) * kTileWidth + (x_tile)*kBoardLineWidth,
          (.5 + y_tile) * kTileHeight + (y_tile)*kBoardLineHeight);
      board_sprite_->addChild(sprite);

      // Modify local game state variables to reflect this most recent move
      board[selected_tile / kTilesX][selected_tile % kTilesX] =
          current_player_index_;
      remaining_tiles_.erase(selected_tile);
      current_player_index_ = (current_player_index_ + 1) % kNumberOfPlayers;
      future_last_move_ = ref_.Child("last_move").SetValue(selected_tile);
      future_current_player_index_ =
          ref_.Child("current_player_index_").SetValue(current_player_index_);
      WaitForCompletion(future_last_move_, "setLastMove");
      WaitForCompletion(future_current_player_index_, "setCurrentPlayerIndex");
      awaiting_opponenet_move_ = true;
      waiting_label_->setString("Waiting");
      if (GameOver(board)) {
        game_outcome_ = kGameWon;
        WaitForCompletion(ref_.Child("game_over").SetValue(true),
                          "setGameOver");
      } else if (remaining_tiles_.size() == 0) {
        // Update game_outcome_ to reflect the user tied.
        game_outcome_ = kGameTied;
        WaitForCompletion(ref_.Child("game_over").SetValue(true),
                          "setGameOver");
      }
    }
    return true;
  };

  Director::getInstance()
      ->getEventDispatcher()
      ->addEventListenerWithSceneGraphPriority(board_touch_listener,
                                               board_sprite_);

  this->addChild(board_sprite_);
}

// If the join_game_uid_ is present, initialize game variables, otherwise
// alter the game variables to signify a user joined. Additionally sets the
// player_index_ and total_players based on joining or creating a game.
void TicTacToeLayer::InitializePlayerData() {
  if (join_game_uid_.empty()) {
    join_game_uid_ = GenerateUid(4);
    ref_ = database_->GetReference("game_data").Child(join_game_uid_);
    future_create_game_ = ref_.Child("total_players").SetValue(1);
    future_current_player_index_ =
        ref_.Child("current_player_index_").SetValue(kPlayerOne);
    future_game_over_ = ref_.Child("game_over").SetValue(false);
    WaitForCompletion(future_game_over_, "setGameOver");
    WaitForCompletion(future_current_player_index_, "setCurrentPlayerIndex");
    WaitForCompletion(future_create_game_, "createGame");
    player_index_ = kPlayerOne;
    awaiting_opponenet_move_ = false;
  } else {
    // Checks whether the join_uid map exists. If it does not then set
    // the initialization to failed.
    auto future_game_uid =
        database_->GetReference("game_data").Child(join_game_uid_).GetValue();
    WaitForCompletion(future_game_uid, "GetGameDataMap");
    auto game_uid_snapshot = future_game_uid.result();
    if (!game_uid_snapshot->value().is_map()) {
      initialization_failed_ = true;
    } else {
      ref_ = database_->GetReference("game_data").Child(join_game_uid_);
      auto future_increment_total_users =
          ref_.RunTransaction([](firebase::database::MutableData* data) {
            auto total_players = data->Child("total_players").value();

            // Completes the transaction based on the returned mutable data
            // value.
            if (total_players.is_null()) {
              // Must return this if the transaction was unsuccessful.
              return TransactionResult::kTransactionResultAbort;
            }
            int new_total_players = total_players.int64_value() + 1;
            if (new_total_players > kNumberOfPlayers) {
              // Must return this if the transaction was unsuccessful.
              return TransactionResult::kTransactionResultAbort;
            }
            data->Child("total_players").set_value(new_total_players);

            // Must call this if the transaction was successful.
            return TransactionResult::kTransactionResultSuccess;
          });
      WaitForCompletion(future_increment_total_users, "JoinGameTransaction");

      player_index_ = kPlayerTwo;
      awaiting_opponenet_move_ = true;
    }
  }
}
// Creates and sets the listeners as follows.
// ExpectedValueListeners: total_players & game_over.
// SampleValueListener: current_player_index & last_move.
void TicTacToeLayer::InitializeDatabaseListeners() {
  // total_player_listener_ and CurrentPlayerIndexListener listener is set up
  // to recognise when the desired players have connected & when turns
  // alternate.
  total_player_listener_ = make_unique<ExpectValueListener>(kNumberOfPlayers);
  game_over_listener_ = make_unique<ExpectValueListener>(true);

  current_player_index_listener_ = make_unique<SampleValueListener>();
  last_move_listener_ = make_unique<SampleValueListener>();

  ref_.Child("total_players").AddValueListener(total_player_listener_.get());
  ref_.Child("game_over").AddValueListener(game_over_listener_.get());
  ref_.Child("current_player_index_")
      .AddValueListener(current_player_index_listener_.get());
  ref_.Child("last_move").AddValueListener(last_move_listener_.get());
}

// Counts down the end_game_frames, and updates the user data based on the game
// outcome.
void TicTacToeLayer::EndGame() {
  // Removes the game from existence and updates the user's record before
  // swap back scenes.
  WaitForCompletion(
      database_->GetReference("game_data").Child(join_game_uid_).RemoveValue(),
      "removeGameUid");
  ref_ = database_->GetReference("users").Child(user_uid_);

  // Updates user record unless the game was disbanded.
  if (game_outcome_ != kGameDisbanded) {
    auto future_record =
        ref_.Child(kGameOutcomeStrings[game_outcome_]).GetValue();
    WaitForCompletion(future_record, "getPreviousOutcomeRecord");
    WaitForCompletion(
        ref_.Child(kGameOutcomeStrings[game_outcome_])
            .SetValue(future_record.result()->value().int64_value() + 1),
        "setGameOutcomeRecord");
  }
  // Pops the scene to return to the previous scene.
  Director::getInstance()->popScene();
}

void TicTacToeLayer::UpdateBoard() {
  int last_move =
      last_move_listener_->last_seen_value().AsInt64().int64_value();

  // Place the players move on the board.
  board[last_move / kTilesX][last_move % kTilesX] = current_player_index_;

  // Remove the tile from the tile unordered set.
  remaining_tiles_.erase(last_move);
  auto sprite = Sprite::create(kPlayerTokenFileNames[current_player_index_]);
  if (sprite == NULL) {
    CCLOG("kPlayerTokenFileNames: %s file not found.",
          kPlayerTokenFileNames[current_player_index_]);
    exit(true);
  }

  // Calculates and sets the position of the sprite based on the
  // move_tile and the constant screen variables.
  const auto x_tile = last_move % kTilesX;
  const auto y_tile = last_move / kTilesY;
  sprite->setPosition((.5 + x_tile) * kTileWidth + (x_tile)*kBoardLineWidth,
                      (.5 + y_tile) * kTileHeight + (y_tile)*kBoardLineHeight);
  board_sprite_->addChild(sprite);

  // Modifies local game state variables to reflect this most recent move.
  board[last_move / kTilesX][last_move % kTilesX] = current_player_index_;
  remaining_tiles_.erase(last_move);
  awaiting_opponenet_move_ = false;
  current_player_index_ = player_index_;
  if (GameOver(board)) {
    // Sets game_outcome_ to reflect the use lost.
    game_outcome_ = kGameLost;
    WaitForCompletion(ref_.Child("game_over").SetValue(true), "setGameOver");
  } else if (remaining_tiles_.size() == 0) {
    // Sets game_outcome_ to reflect the game ended in a tie.
    game_outcome_ = kGameTied;
    WaitForCompletion(ref_.Child("game_over").SetValue(true), "setGameOver");
  }
}

void TicTacToeLayer::DisplayGameOutcome() {
  displaying_outcome_ = true;

  // Checks to see if the opponenet rage quit.
  if (game_outcome_ == kGameDisbanded &&
      remaining_tiles_.size() != kNumberOfTiles) {
    game_outcome_ = kGameWon;
  }

  // Creates the delay action.
  auto loading_delay = DelayTime::create(/*delay_durration*/ 2.0f);

  // Creates a callable function for EndGame().
  auto RunEndGame = CallFunc::create([this]() { this->EndGame(); });

  // Runs the sequence that will delay followed by calling EndGame().
  this->runAction(Sequence::create(loading_delay, RunEndGame, NULL));

  // Creates and displays the game outcome image.
  const auto end_game_image = Sprite::create(kGameOutcomeImages[game_outcome_]);
  end_game_image->setPosition(Vec2(300, 300));
  this->addChild(end_game_image);
}

TicTacToeLayer::~TicTacToeLayer() {
  // releases our sprite and layer so that it gets deallocated
  CC_SAFE_RELEASE_NULL(this->board_sprite_);
  CC_SAFE_RELEASE_NULL(this->waiting_label_);
}