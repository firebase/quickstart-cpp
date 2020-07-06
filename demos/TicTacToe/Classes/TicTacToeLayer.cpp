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

#include "TicTacToeLayer.h"

USING_NS_CC;

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

// create an array that has indicies of enum kGameOutcome and maps that to the
// database outcome key.
static const const char* kGameOutcomeStrings[] = {"wins", "loses", "ties",
                                                  "disbanded"};
static const std::array<string, 4> kGameOverStrings = {
    "you won!", "you lost.", "you tied.", "user left."};

// Game board dimensions.
extern const int kTilesX;
extern const int kTilesY;
static const int kNumberOfTiles = kTilesX * kTilesY;

// Screen dimensions.
static const double kScreenWidth = 600;
static const double kScreenHeight = 600;
static const double kTileWidth = (kScreenWidth / kTilesX);
static const double kTileHeight = (kScreenHeight / kTilesY);

// The screen will display the end game text for 2 seconds (120frames/60fps).
static const int kEndGameFramesMax = 120;

// Image file paths.
static const char* kBoardImageFileName = "tic_tac_toe_board.png";
static const char* kLeaveButtonFileName = "leave_button.png";
static std::array<const char*, kNumberOfPlayers> kPlayerTokenFileNames = {
    "tic_tac_toe_x.png", "tic_tac_toe_o.png"};

// An example of a ValueListener object. This specific version will
// simply log every value it sees, and store them in a list so we can
// confirm that all values were received.
class SampleValueListener : public firebase::database::ValueListener {
 public:
  void OnValueChanged(
      const firebase::database::DataSnapshot& snapshot) override {
    LogMessage("  ValueListener.OnValueChanged(%s)",
               snapshot.value().AsString().string_value());
    last_seen_value_ = snapshot.value();
    seen_values_.push_back(snapshot.value());
  }
  void OnCancelled(const firebase::database::Error& error_code,
                   const char* error_message) override {
    LogMessage("ERROR: SampleValueListener canceled: %d: %s", error_code,
               error_message);
  }
  const firebase::Variant& last_seen_value() { return last_seen_value_; }
  bool seen_value(const firebase::Variant& value) {
    return std::find(seen_values_.begin(), seen_values_.end(), value) !=
           seen_values_.end();
  }
  size_t num_seen_values() { return seen_values_.size(); }

 private:
  firebase::Variant last_seen_value_;
  std::vector<firebase::Variant> seen_values_;
};

// An example ChildListener class.
class SampleChildListener : public firebase::database::ChildListener {
 public:
  void OnChildAdded(const firebase::database::DataSnapshot& snapshot,
                    const char* previous_sibling) override {
    LogMessage("  ChildListener.OnChildAdded(%s)", snapshot.key());
    events_.push_back(std::string("added ") + snapshot.key());
  }
  void OnChildChanged(const firebase::database::DataSnapshot& snapshot,
                      const char* previous_sibling) override {
    LogMessage("  ChildListener.OnChildChanged(%s)", snapshot.key());
    events_.push_back(std::string("changed ") + snapshot.key());
  }
  void OnChildMoved(const firebase::database::DataSnapshot& snapshot,
                    const char* previous_sibling) override {
    LogMessage("  ChildListener.OnChildMoved(%s)", snapshot.key());
    events_.push_back(std::string("moved ") + snapshot.key());
  }
  void OnChildRemoved(
      const firebase::database::DataSnapshot& snapshot) override {
    LogMessage("  ChildListener.OnChildRemoved(%s)", snapshot.key());
    events_.push_back(std::string("removed ") + snapshot.key());
  }
  void OnCancelled(const firebase::database::Error& error_code,
                   const char* error_message) override {
    LogMessage("ERROR: SampleChildListener canceled: %d: %s", error_code,
               error_message);
  }

  // Get the total number of Child events this listener saw.
  size_t total_events() { return events_.size(); }

  // Get the number of times this event was seen.
  int num_events(const std::string& event) {
    int count = 0;
    for (int i = 0; i < events_.size(); i++) {
      if (events_[i] == event) {
        count++;
      }
    }
    return count;
  }

 public:
  // Vector of strings that define the events we saw, in order.
  std::vector<std::string> events_;
};

// A ValueListener that expects a specific value to be set.
class ExpectValueListener : public firebase::database::ValueListener {
 public:
  explicit ExpectValueListener(firebase::Variant wait_value)
      : wait_value_(wait_value.AsString()), got_value_(false) {}
  void OnValueChanged(
      const firebase::database::DataSnapshot& snapshot) override {
    if (snapshot.value().AsString() == wait_value_) {
      got_value_ = true;
    } else {
      LogMessage(
          "FAILURE: ExpectValueListener did not receive the expected result.");
    }
  }
  void OnCancelled(const firebase::database::Error& error_code,
                   const char* error_message) override {
    LogMessage("ERROR: ExpectValueListener canceled: %d: %s", error_code,
               error_message);
  }

  bool got_value() { return got_value_; }

 private:
  firebase::Variant wait_value_;
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

TicTacToeLayer::TicTacToeLayer(string game_uuid,
                               firebase::database::Database* main_menu_database,
                               string main_menu_user) {
  join_game_uuid_ = game_uuid;
  current_player_index_ = kPlayerOne;
  game_outcome_ = kGameWon;
  database_ = main_menu_database;
  user_uid_ = main_menu_user;

  // If the join_game_uuid_ is present, initialize game variables, otherwise
  // alter the game variables to signify a user joined. Additionally sets the
  // player_index_ and total_players based on joining or creating a game.
  if (join_game_uuid_.empty()) {
    join_game_uuid_ = GenerateUid(4);
    ref_ = database_->GetReference("game_data").Child(join_game_uuid_);
    firebase::Future<void> future_create_game =
        ref_.Child("total_players").SetValue(1);
    future_current_player_index_ =
        ref_.Child("current_player_index_").SetValue(kPlayerOne);
    future_game_over_ = ref_.Child("game_over").SetValue(false);
    WaitForCompletion(future_game_over_, "setGameOver");
    WaitForCompletion(future_current_player_index_, "setCurrentPlayerIndex");
    WaitForCompletion(future_create_game, "createGame");
    player_index_ = kPlayerOne;
    awaiting_opponenet_move_ = false;
  } else {
    // Checks whether the join_uuid map exists. If it does not then set
    // the initialization to failed.
    auto future_game_uuid =
        database_->GetReference("game_data").Child(join_game_uuid_).GetValue();
    WaitForCompletion(future_game_uuid, "GetGameDataMap");
    auto game_uuid_snapshot = future_game_uuid.result();
    if (!game_uuid_snapshot->value().is_map()) {
      initialization_failed_ = true;
    } else {
      ref_ = database_->GetReference("game_data").Child(join_game_uuid_);
      auto future_increment_total_users =
          ref_.RunTransaction([](MutableData* data) {
            auto total_players = data->Child("total_players").value();

            // Complete the transaction based on the returned mutable data
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

  // Create the board sprite , set the position to the bottom left of the
  // frame (0,0), and finally move the anchor point from the center of the
  // image(default) to the bottom left, Vec2(0.0,0.0).
  board_sprite_ = Sprite::create(kBoardImageFileName);
  if (!board_sprite_) {
    log("kBoardImageFileName: %s file not found.", kBoardImageFileName);
    exit(true);
  }
  board_sprite_->setPosition(0, 0);
  board_sprite_->setAnchorPoint(Vec2(0.0, 0.0));

  leave_button_sprite_ = Sprite::create(kLeaveButtonFileName);
  if (!leave_button_sprite_) {
    log("kLeaveButtonSprite: %s file not found.", kLeaveButtonFileName);
    exit(true);
  }
  leave_button_sprite_->setPosition(450, 585);
  leave_button_sprite_->setAnchorPoint(Vec2(0.0, 0.0));
  leave_button_sprite_->setScale(.35);

  // Create a button listener to handle the touch event.
  auto leave_button_sprite_touch_listener =
      EventListenerTouchOneByOne::create();

  // Set the onTouchBegan event up to a lambda will swap scenes and modify
  // total_players
  leave_button_sprite_touch_listener->onTouchBegan =
      [this](Touch* touch, Event* event) -> bool {
    const auto bounds = event->getCurrentTarget()->getBoundingBox();
    const auto point = touch->getLocation();

    // Replaces the scene with a new TicTacToe scene if the touched point is
    // within the bounds of the button.
    if (bounds.containsPoint(point)) {
      // Update the game_outcome_ to reflect if the you rage quit or left
      // pre-match.
      if (remaining_tiles_.size() == kNumberOfTiles) {
        game_outcome_ = kGameDisbanded;
      } else {
        game_outcome_ = kGameLost;
      }

      WaitForCompletion(ref_.Child("game_over").SetValue(true), "setGameOver");
    }

    return true;
  };

  // Attach the touch listener to the create game button.
  Director::getInstance()
      ->getEventDispatcher()
      ->addEventListenerWithSceneGraphPriority(
          leave_button_sprite_touch_listener, leave_button_sprite_);

  board_sprite_->addChild(leave_button_sprite_, /*layer_index=*/1);

  // TODO(grantpostma@): Modify these numbers to be based on the extern window
  // size & label size dimensions.
  cocos2d::Label* game_uuid_label =
      Label::createWithSystemFont(join_game_uuid_, "Arial", 30);
  game_uuid_label->setPosition(Vec2(40, 20));
  board_sprite_->addChild(game_uuid_label, /*layer_index=*/1);
  waiting_label_ = Label::createWithSystemFont("waiting", "Arial", 30);
  waiting_label_->setPosition(Vec2(530, 20));

  board_sprite_->addChild(waiting_label_, /*layer_index=*/1);
  game_over_label_ = Label::createWithSystemFont("", "Arial", 80);
  game_over_label_->setPosition(Vec2(300, 300));
  board_sprite_->addChild(game_over_label_, /*layer_index=*/1);

  // total_player_listener_ and CurrentPlayerIndexListener listener is set up
  // to recognise when the desired players have connected & when turns
  // alternate.
  total_player_listener_ =
      std::make_unique<ExpectValueListener>(kNumberOfPlayers);
  game_over_listener_ = std::make_unique<ExpectValueListener>(true);

  current_player_index_listener_ = std::make_unique<SampleValueListener>();
  last_move_listener_ = std::make_unique<SampleValueListener>();
  ref_.Child("total_players").AddValueListener(total_player_listener_.get());
  ref_.Child("game_over").AddValueListener(game_over_listener_.get());
  ref_.Child("current_player_index_")
      .AddValueListener(current_player_index_listener_.get());
  ref_.Child("last_move").AddValueListener(last_move_listener_.get());

  // Set up a 3*3 Tic-Tac-Toe board for tracking results.
  for (int i = 0; i < kTilesY; i++) {
    for (int j = 0; j < kTilesX; j++) {
      board[i][j] = kEmptyTile;
      remaining_tiles_.insert((i * kTilesX) + j);
    };
  }

  // Add a function to determine which tile was selected to the onTouchBegan
  // listener.
  auto touch_listener = EventListenerTouchOneByOne::create();
  touch_listener->onTouchBegan = [this](Touch* touch,
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
      int selected_tile = floor(touch->getLocation().x / kTileWidth) +
                          kTilesX * floor(touch->getLocation().y / kTileHeight);
      if (remaining_tiles_.find(selected_tile) == remaining_tiles_.end())
        return true;

      auto sprite =
          Sprite::create(kPlayerTokenFileNames[current_player_index_]);
      if (sprite == NULL) {
        log("kPlayerTokenFileNames: %s file not found.",
            kPlayerTokenFileNames[current_player_index_]);
        exit(true);
      }

      // Calculates and sets the position of the sprite based on the
      // move_tile and the constant screen variables.
      sprite->setPosition((.5 + selected_tile % kTilesX) * kTileWidth,
                          (.5 + selected_tile / kTilesY) * kTileHeight);
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
      waiting_label_->setString("waiting");
      if (GameOver(board)) {
        // Set game_over_label_ to reflect the user won.
        game_over_label_->setString("you won!");
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
      ->addEventListenerWithSceneGraphPriority(touch_listener, board_sprite_);

  this->addChild(board_sprite_);

  // Schedule the update method for this scene.
  this->scheduleUpdate();
}

// Called automatically every frame. The update is scheduled in constructor.
void TicTacToeLayer::update(float /*delta*/) {
  // Replace the scene with MainMenuScene if the initialization fails.
  if (initialization_failed_ == true) {
    Director::getInstance()->popScene();
  }

  // Performs the actions of the other player when the
  // current_player_index_listener_ is equal to the player index.
  else if (current_player_index_listener_->last_seen_value() == player_index_ &&
           awaiting_opponenet_move_ == true) {
    int last_move =
        last_move_listener_->last_seen_value().AsInt64().int64_value();

    // Place the players move on the board.
    board[last_move / kTilesX][last_move % kTilesX] = current_player_index_;

    // Remove the tile from the tile unordered set.
    remaining_tiles_.erase(last_move);
    auto sprite = Sprite::create(kPlayerTokenFileNames[current_player_index_]);
    if (sprite == NULL) {
      log("kPlayerTokenFileNames: %s file not found.",
          kPlayerTokenFileNames[current_player_index_]);
      exit(true);
    }

    // Calculates and sets the position of the sprite based on the
    // move_tile and the constant screen variables.
    sprite->setPosition((.5 + last_move % kTilesX) * kTileWidth,
                        (.5 + last_move / kTilesY) * kTileHeight);
    board_sprite_->addChild(sprite);

    // Modify local game state variables to reflect this most recent move.
    board[last_move / kTilesX][last_move % kTilesX] = current_player_index_;
    remaining_tiles_.erase(last_move);
    awaiting_opponenet_move_ = false;
    current_player_index_ = player_index_;
    if (GameOver(board)) {
      // Set game_outcome_ to reflect the use lost.
      game_outcome_ = kGameLost;
      WaitForCompletion(ref_.Child("game_over").SetValue(true), "setGameOver");
    } else if (remaining_tiles_.size() == 0) {
      // Set game_outcome_ to reflect the game ended in a tie.
      game_outcome_ = kGameTied;
      WaitForCompletion(ref_.Child("game_over").SetValue(true), "setGameOver");
    }
  }

  // Shows the end game label for kEndGameFramesMax to show the result of the
  // game.
  else if (game_over_listener_->got_value()) {
    if (game_outcome_ == kGameDisbanded &&
        remaining_tiles_.size() != kNumberOfTiles) {
      game_outcome_ = kGameWon;
    }
    game_over_label_->setString(kGameOverStrings[game_outcome_]);
    end_game_frames_++;
    if (end_game_frames_ > kEndGameFramesMax) {
      // Remove the game from existence and update the user's record before
      // swap back scenes.
      WaitForCompletion(database_->GetReference("game_data")
                            .Child(join_game_uuid_)
                            .RemoveValue(),
                        "removeGameUuid");
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

      Director::getInstance()->popScene();
    }
  }

  // Updates the waiting label to signify it is this players move.
  else if (total_player_listener_->got_value() &&
           awaiting_opponenet_move_ == false) {
    waiting_label_->setString("your move");
  }
}

TicTacToeLayer::~TicTacToeLayer() {
  // release our sprite and layer so that it gets deallocated
  CC_SAFE_RELEASE_NULL(this->board_sprite_);
  CC_SAFE_RELEASE_NULL(this->game_over_label_);
  CC_SAFE_RELEASE_NULL(this->waiting_label_);
}
