#include "TicTacToeScene.h"

#include "MainMenuScene.h"
#include "cocos2d.h"
#include "firebase/app.h"
#include "firebase/auth.h"
#include "firebase/database.h"

using firebase::database::DataSnapshot;
using firebase::database::MutableData;
using firebase::database::TransactionResult;
#include "firebase/future.h"
#include "firebase/util.h"

// Thin OS abstraction layer.
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

USING_NS_CC;

// Player constants.
static const int kEmptyTile = -1;
static const int kPlayerOne = 0;
static const int kPlayerTwo = 1;
static const int kNumberOfPlayers = 2;
// Game board dimensions.
static const int kTilesX = 3;
static const int kTilesY = 3;
static const int kNumberOfTiles = kTilesX * kTilesY;
// Screen dimensions.
static const double kScreenWidth = 600;
static const double kScreenHeight = 600;
static const double kTileWidth = (kScreenWidth / kTilesX);
static const double kTileHeight = (kScreenHeight / kTilesY);
// Image file paths.
static const char* kBoardImageFileName = "tic_tac_toe_board.png";
static std::array<const char*, kNumberOfPlayers> kPlayerTokenFileNames = {
    "tic_tac_toe_x.png", "tic_tac_toe_o.png"};

// four letter random code to join the game.
char* join_game_uuid = NULL;
firebase::database::DatabaseReference ref;
// Creating lables and a sprite `for the board
Sprite* board_sprite;
cocos2d::Label* game_over_label;
cocos2d::Label* waiting_label;
// Creating firebase futures for last_move and current_player_index
firebase::Future<void> fLastMove;
firebase::Future<void> fCurrentPlayerIndex;

// Creating the board, remaining available tile set and player index variables.
int board[kTilesX][kTilesY];
std::unordered_set<int> remaining_tiles;
int current_player_index = kPlayerOne;
int player_index;
bool awaiting_opponenet_move;

void LogMessage(const char* format, ...) {
  va_list list;
  va_start(list, format);
  vprintf(format, list);
  va_end(list);
  printf("\n");
  fflush(stdout);
}

bool ProcessEvents(int msec) {
#ifdef _WIN32
  Sleep(msec);
#else
  usleep(msec * 1000);
#endif  // _WIN32
  return false;
}

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
  // Vector of strings defining the events we saw, in order.
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

// Wait for a Future to be completed. If the Future returns an error, it will
// be logged.
void WaitForCompletion(const firebase::FutureBase& future, const char* name) {
  while (future.status() == firebase::kFutureStatusPending) {
    ProcessEvents(100);
  }
  if (future.status() != firebase::kFutureStatusComplete) {
    LogMessage("ERROR: %s returned an invalid result.", name);
  } else if (future.error() != 0) {
    LogMessage("ERROR: %s returned error %d: %s", name, future.error(),
               future.error_message());
  }
}
char* random_string(std::size_t length) {
  const std::string CHARACTERS = "0123456789abcdefghijkmnopqrstuvwxyz";

  std::random_device random_device;
  std::mt19937 generator(random_device());
  std::uniform_int_distribution<> distribution(0, CHARACTERS.size() - 1);

  char* random_string = new char[length + 1];

  for (std::size_t i = 0; i < length; ++i) {
    random_string[i] = CHARACTERS[distribution(generator)];
  }
  random_string[length] = '\0';

  return random_string;
}

// A function that returns true if any of the row
// is crossed with the same player's move
bool rowCrossed(int board[][kTilesY]) {
  for (int i = 0; i < kTilesY; i++) {
    if (board[i][0] == board[i][1] && board[i][1] == board[i][2] &&
        board[i][0] != kEmptyTile)
      return (true);
  }
  return (false);
}

// A function that returns true if any of the column
// is crossed with the same player's move
bool columnCrossed(int board[][kTilesY]) {
  for (int i = 0; i < kTilesX; i++) {
    if (board[0][i] == board[1][i] && board[1][i] == board[2][i] &&
        board[0][i] != kEmptyTile)
      return (true);
  }
  return (false);
}

// A function that returns true if any of the diagonal
// is crossed with the same player's move
bool diagonalCrossed(int board[][kTilesY]) {
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
bool gameOver(int board[][kTilesY]) {
  return (rowCrossed(board) || columnCrossed(board) || diagonalCrossed(board));
}

// Creating listeners for database values.
SampleValueListener* current_player_index_listener;
SampleValueListener* last_move_listener;
ExpectValueListener* total_player_listener;

Scene* TicTacToe::createScene(char* game_uuid) {
  // Sets the join_game_uuid to the passed in game_uuid.
  join_game_uuid = game_uuid;
  // Builds a simple scene that uses the bottom left cordinate point as (0,0)
  // and can have sprites, labels and layers added onto it.
  Scene* scene = Scene::create();

  // Builds a layer to be placed onto the scene which has access to TouchEvents.
  TicTacToe* tic_tac_toe_layer = TicTacToe::create();

  scene->addChild(tic_tac_toe_layer);

  return scene;
}

bool TicTacToe::init() {
  if (!Layer::init()) {
    return false;
  }
  LogMessage("Initialized Firebase App.");
  auto app = ::firebase::App::Create();
  LogMessage("Initialize Firebase Auth and Firebase Database.");
  // Use ModuleInitializer to initialize both Auth and Database, ensuring no
  // dependencies are missing.
  firebase::ModuleInitializer initializer;

  /// Firebase Auth, used for logging into Firebase.
  firebase::auth::Auth* auth;

  /// Firebase Realtime Database, the entry point to all database operations.
  firebase::database::Database* database;

  database = nullptr;
  auth = nullptr;
  void* initialize_targets[] = {&auth, &database};

  const firebase::ModuleInitializer::InitializerFn initializers[] = {
      [](::firebase::App* app, void* data) {
        LogMessage("Attempt to initialize Firebase Auth.");
        void** targets = reinterpret_cast<void**>(data);
        ::firebase::InitResult result;
        *reinterpret_cast<::firebase::auth::Auth**>(targets[0]) =
            ::firebase::auth::Auth::GetAuth(app, &result);
        return result;
      },
      [](::firebase::App* app, void* data) {
        LogMessage("Attempt to initialize Firebase Database.");
        void** targets = reinterpret_cast<void**>(data);
        ::firebase::InitResult result;
        *reinterpret_cast<::firebase::database::Database**>(targets[1]) =
            ::firebase::database::Database::GetInstance(app, &result);
        return result;
      }};

  initializer.Initialize(app, initialize_targets, initializers,
                         sizeof(initializers) / sizeof(initializers[0]));

  WaitForCompletion(initializer.InitializeLastResult(), "Initialize");

  if (initializer.InitializeLastResult().error() != 0) {
    LogMessage("Failed to initialize Firebase libraries: %s",
               initializer.InitializeLastResult().error_message());
    ProcessEvents(2000);
    return 1;
  }
  LogMessage("Successfully initialized Firebase Auth and Firebase Database.");

  database->set_persistence_enabled(true);

  // Sign in using Auth before accessing the database.
  // The default Database permissions allow anonymous users access. This will
  // work as long as your project's Authentication permissions allow anonymous
  // signin.
  {
    firebase::Future<firebase::auth::User*> sign_in_future =
        auth->SignInAnonymously();
    WaitForCompletion(sign_in_future, "SignInAnonymously");
    if (sign_in_future.error() == firebase::auth::kAuthErrorNone) {
      LogMessage("Auth: Signed in anonymously.");
    } else {
      LogMessage("ERROR: Could not sign in anonymously. Error %d: %s",
                 sign_in_future.error(), sign_in_future.error_message());
      LogMessage(
          "  Ensure your application has the Anonymous sign-in provider "
          "enabled in Firebase Console.");
      LogMessage(
          "  Attempting to connect to the database anyway. This may fail "
          "depending on the security settings.");
    }
  }
  // Splits on the if depending on if the player created or joined the game.
  // Additionally sets the player_index and total_players based on joining or
  // creating a game.
  if (join_game_uuid == NULL) {
    join_game_uuid = random_string(4);
    ref = database->GetReference("game_data").Child(join_game_uuid);
    firebase::Future<void> fCreateGame = ref.Child("total_players").SetValue(1);
    fCurrentPlayerIndex =
        ref.Child("current_player_index").SetValue(kPlayerOne);
    WaitForCompletion(fCurrentPlayerIndex, "setCurrentPlayerIndex");
    WaitForCompletion(fCreateGame, "createGame");
    player_index = kPlayerOne;
    awaiting_opponenet_move = false;
  } else {
    ref = database->GetReference("game_data").Child(join_game_uuid);
    auto fIncrementTotalUsers = ref.RunTransaction([](MutableData* data) {
      auto total_players = data->Child("total_players").value();
      if (total_players.type() == NULL) {
        return TransactionResult::kTransactionResultSuccess;
      }
      int new_total_players = total_players.int64_value() + 1;
      if (new_total_players > kNumberOfPlayers) {
        // Callback function needs to replace Scenes
        return TransactionResult::kTransactionResultAbort;
      }
      data->Child("total_players").set_value(new_total_players);
      // Must call this if the transaction was successful.
      return TransactionResult::kTransactionResultSuccess;
    });
    WaitForCompletion(fIncrementTotalUsers, "JoinGameTransaction");

    player_index = kPlayerTwo;
    awaiting_opponenet_move = true;
  }

  // Creating the board sprite , setting the position to the bottom left of the
  // frame (0,0), and finally moving the anchor point from the center of the
  // image(default) to the bottom left, Vec2(0.0,0.0).
  board_sprite = Sprite::create(kBoardImageFileName);
  if (!board_sprite) {
    log("kBoardImageFileName: %s file not found.", kBoardImageFileName);
    exit(true);
  }
  board_sprite->setPosition(0, 0);
  board_sprite->setAnchorPoint(Vec2(0.0, 0.0));

  // TODO(grantpostma@): Modify these numbers to be based on the extern window
  // size & label size dimensions.
  cocos2d::Label* game_uuid_label =
      Label::createWithSystemFont(join_game_uuid, "Arial", 30);
  game_uuid_label->setPosition(Vec2(40, 20));
  board_sprite->addChild(game_uuid_label, 1);
  waiting_label = Label::createWithSystemFont("waiting", "Arial", 30);
  waiting_label->setPosition(Vec2(530, 20));

  board_sprite->addChild(waiting_label, 1);
  game_over_label = Label::createWithSystemFont("", "Arial", 80);
  game_over_label->setPosition(Vec2(300, 300));
  board_sprite->addChild(game_over_label, 1);

  // total_player_listener and CurrentPlayerIndexListener listener is set up
  // to recognise when the desired players have connected & when turns
  // alternate
  LogMessage("total_player_listener");
  total_player_listener = new ExpectValueListener(kNumberOfPlayers);
  current_player_index_listener = new SampleValueListener();
  last_move_listener = new SampleValueListener();

  ref.Child("total_players").AddValueListener(total_player_listener);
  ref.Child("current_player_index")
      .AddValueListener(current_player_index_listener);
  ref.Child("last_move").AddValueListener(last_move_listener);

  // A 3*3 Tic-Tac-Toe board for playing
  for (int i = 0; i < kTilesY; i++) {
    for (int j = 0; j < kTilesX; j++) {
      board[i][j] = kEmptyTile;
      remaining_tiles.insert((i * kTilesX) + j);
    };
  }
  // Adding a function to determine which tile was selected to the onTouchBegan
  // listener.
  auto touch_listener = EventListenerTouchOneByOne::create();
  touch_listener->onTouchBegan = [](Touch* touch,
                                    Event* event) mutable -> bool {
    if (!total_player_listener->got_value()) return true;
    if (current_player_index_listener->last_seen_value() != player_index)
      return true;

    auto bounds = event->getCurrentTarget()->getBoundingBox();
    // Checking to make sure the touch location is within the bounds of the
    // board.
    if (bounds.containsPoint(touch->getLocation())) {
      // Calculates the tile number [0-8] which corresponds to the touch
      // location.
      int selected_tile = floor(touch->getLocation().x / kTileWidth) +
                          kTilesX * floor(touch->getLocation().y / kTileHeight);
      if (remaining_tiles.find(selected_tile) == remaining_tiles.end())
        return true;

      auto sprite = Sprite::create(kPlayerTokenFileNames[current_player_index]);
      if (sprite == NULL) {
        log("kPlayerTokenFileNames: %s file not found.",
            kPlayerTokenFileNames[current_player_index]);
        exit(true);
      }
      // Calculates and sets the position of the sprite based on the
      // move_tile and the constant screen variables.
      sprite->setPosition((.5 + selected_tile % kTilesX) * kTileWidth,
                          (.5 + selected_tile / kTilesY) * kTileHeight);
      board_sprite->addChild(sprite);
      // Modifying local game state variables to reflect this most recent move
      board[selected_tile / kTilesX][selected_tile % kTilesX] =
          current_player_index;
      remaining_tiles.erase(selected_tile);
      current_player_index = (current_player_index + 1) % kNumberOfPlayers;
      fLastMove = ref.Child("last_move").SetValue(selected_tile);
      fCurrentPlayerIndex =
          ref.Child("current_player_index").SetValue(current_player_index);
      WaitForCompletion(fLastMove, "setLastMove");
      WaitForCompletion(fCurrentPlayerIndex, "setCurrentPlayerIndex");
      awaiting_opponenet_move = true;
      waiting_label->setString("waiting");
      if (gameOver(board)) {
        // Set game_over_label to reflect the use won.
        game_over_label->setString("you won.");
      }
      if (remaining_tiles.size() == 0) {
        // Set game_over_label to reflect the game ended in a tie.
        game_over_label->setString("you tied.");
        ProcessEvents(3000);
        // Changing back to the main menu scene.
        Director::getInstance()->replaceScene(MainMenuScene::createScene());
      }
    }
    return true;
  };

  Director::getInstance()
      ->getEventDispatcher()
      ->addEventListenerWithSceneGraphPriority(touch_listener, board_sprite);

  this->addChild(board_sprite);
  // Schedule the update method for this scene.
  this->scheduleUpdate();

  return true;
}

// Called automatically every frame. The update is scheduled in `init()`.
void TicTacToe::update(float /*delta*/) {
  // Performs the actions of the other player when the
  // current_player_index_listener is equal to the player index.
  if (current_player_index_listener->last_seen_value() == player_index &&
      awaiting_opponenet_move == true) {
    int last_move =
        last_move_listener->last_seen_value().AsInt64().int64_value();
    // Placing the players move on the board.
    board[last_move / kTilesX][last_move % kTilesX] = current_player_index;
    // Removing the tile from the tile unordered set.
    remaining_tiles.erase(last_move);
    auto sprite = Sprite::create(kPlayerTokenFileNames[current_player_index]);
    if (sprite == NULL) {
      log("kPlayerTokenFileNames: %s file not found.",
          kPlayerTokenFileNames[current_player_index]);
      exit(true);
    }
    // Calculates and sets the position of the sprite based on the
    // move_tile and the constant screen variables.
    sprite->setPosition((.5 + last_move % kTilesX) * kTileWidth,
                        (.5 + last_move / kTilesY) * kTileHeight);
    board_sprite->addChild(sprite);
    // Modifying local game state variables to reflect this most recent move.
    board[last_move / kTilesX][last_move % kTilesX] = current_player_index;
    remaining_tiles.erase(last_move);
    awaiting_opponenet_move = false;
    current_player_index = player_index;
    if (gameOver(board)) {
      // Set game_over_label to reflect the use lost.
      game_over_label->setString("you lost!");
    }
    if (remaining_tiles.size() == 0) {
      // Set game_over_label to reflect the game ended in a tie.
      game_over_label->setString("you tied.");
    }
  }
  // Updates the waiting label to signify it is this players move.
  else if (total_player_listener->got_value() &&
           awaiting_opponenet_move == false) {
    waiting_label->setString("your move");
  }
}
