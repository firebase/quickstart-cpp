// Copyright 2016 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <algorithm>
#include <ctime>

#include "firebase/app.h"
#include "firebase/auth.h"
#include "firebase/database.h"
#include "firebase/future.h"
#include "firebase/util.h"


// Thin OS abstraction layer.
#include "main.h"  // NOLINT

// Tic-Tac-Toe required includes
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>

// Tic-Tac-Toe define statements (Should be moved into Tic-Tac-Toe class)
#define COMPUTER 0
#define HUMAN 1
#define SIDE 3
#define COMPUTERMOVE 'O'
#define HUMANMOVE 'X'

// confirm that all values were received.
class SampleValueListener : public firebase::database::ValueListener {
 public:
  void OnValueChanged(
      const firebase::database::DataSnapshot &snapshot) override {
    LogMessage("  ValueListener.OnValueChanged(%s)",
               snapshot.value().AsString().string_value());
    last_seen_value_ = snapshot.value();
    seen_values_.push_back(snapshot.value());
  }
  void OnCancelled(const firebase::database::Error &error_code,
                   const char *error_message) override {
    LogMessage("ERROR: SampleValueListener canceled: %d: %s", error_code,
               error_message);
  }
  const firebase::Variant &last_seen_value() { return last_seen_value_; }
  bool seen_value(const firebase::Variant &value) {
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
  void OnChildAdded(const firebase::database::DataSnapshot &snapshot,
                    const char *previous_sibling) override {
    LogMessage("  ChildListener.OnChildAdded(%s)", snapshot.key());
    events_.push_back(std::string("added ") + snapshot.key());
  }
  void OnChildChanged(const firebase::database::DataSnapshot &snapshot,
                      const char *previous_sibling) override {
    LogMessage("  ChildListener.OnChildChanged(%s)", snapshot.key());
    events_.push_back(std::string("changed ") + snapshot.key());
  }
  void OnChildMoved(const firebase::database::DataSnapshot &snapshot,
                    const char *previous_sibling) override {
    LogMessage("  ChildListener.OnChildMoved(%s)", snapshot.key());
    events_.push_back(std::string("moved ") + snapshot.key());
  }
  void OnChildRemoved(
      const firebase::database::DataSnapshot &snapshot) override {
    LogMessage("  ChildListener.OnChildRemoved(%s)", snapshot.key());
    events_.push_back(std::string("removed ") + snapshot.key());
  }
  void OnCancelled(const firebase::database::Error &error_code,
                   const char *error_message) override {
    LogMessage("ERROR: SampleChildListener canceled: %d: %s", error_code,
               error_message);
  }

  // Get the total number of Child events this listener saw.
  size_t total_events() { return events_.size(); }

  // Get the number of times this event was seen.
  int num_events(const std::string &event) {
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
      const firebase::database::DataSnapshot &snapshot) override {
    if (snapshot.value().AsString() == wait_value_) {
      got_value_ = true;
    } else {
      LogMessage(
          "FAILURE: ExpectValueListener did not receive the expected result.");
    }
  }
  void OnCancelled(const firebase::database::Error &error_code,
                   const char *error_message) override {
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
void WaitForCompletion(const firebase::FutureBase &future, const char *name) {
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

// A function to show the current board status
void showBoard(char board[][SIDE]) {
  printf("\n\n");

  printf("\t\t\t %c | %c | %c \n", board[0][0], board[0][1], board[0][2]);
  printf("\t\t\t--------------\n");
  printf("\t\t\t %c | %c | %c \n", board[1][0], board[1][1], board[1][2]);
  printf("\t\t\t--------------\n");
  printf("\t\t\t %c | %c | %c \n\n", board[2][0], board[2][1], board[2][2]);

  return;
}

// A function to show the instructions
void showInstructions() {
  printf("\t\t\t Tic-Tac-Toe\n\n");
  printf(
      "Choose a cell numbered from 1 to 9 as below"
      " and play\n\n");

  printf("\t\t\t 1 | 2 | 3 \n");
  printf("\t\t\t--------------\n");
  printf("\t\t\t 4 | 5 | 6 \n");
  printf("\t\t\t--------------\n");
  printf("\t\t\t 7 | 8 | 9 \n\n");

  printf("-\t-\t-\t-\t-\t-\t-\t-\t-\t-\n\n");

  return;
}

void printSet(std::unordered_set<int> const &us) {
  std::set<int> s;
  for (auto move : us) {
    s.insert(move);
  }
  std::copy(s.begin(), s.end(), std::ostream_iterator<int>(std::cout, " "));
}

// A function to initialise the game
void initialise(char board[][SIDE], std::unordered_set<int> &us) {
  // Initiate the random number generator so that
  // the same configuration doesn't arises
  int moves[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  srand(time(NULL));

  // Initially the board is empty
  for (int i = 0; i < SIDE; i++) {
    for (int j = 0; j < SIDE; j++) board[i][j] = ' ';
  }

  // randomise the moves
  // random_shuffle(moves, moves + SIDE*SIDE);
  std::unordered_set<int> ms(moves, moves + 9);
  us = ms;

  return;
}

// A function to declare the winner of the game
void declareWinner(int whoseTurn, int playerNumber) {
  if (whoseTurn == playerNumber)
    printf("You have won\n");
  else
    printf("You have lost\n");
  return;
}

// A function that returns true if any of the row
// is crossed with the same player's move
bool rowCrossed(char board[][SIDE]) {
  for (int i = 0; i < SIDE; i++) {
    if (board[i][0] == board[i][1] && board[i][1] == board[i][2] &&
        board[i][0] != ' ')
      return (true);
  }
  return (false);
}

// A function that returns true if any of the column
// is crossed with the same player's move
bool columnCrossed(char board[][SIDE]) {
  for (int i = 0; i < SIDE; i++) {
    if (board[0][i] == board[1][i] && board[1][i] == board[2][i] &&
        board[0][i] != ' ')
      return (true);
  }
  return (false);
}

// A function that returns true if any of the diagonal
// is crossed with the same player's move
bool diagonalCrossed(char board[][SIDE]) {
  if (board[0][0] == board[1][1] && board[1][1] == board[2][2] &&
      board[0][0] != ' ')
    return (true);

  if (board[0][2] == board[1][1] && board[1][1] == board[2][0] &&
      board[0][2] != ' ')
    return (true);

  return (false);
}

// A function that returns true if the game is over
// else it returns a false
bool gameOver(char board[][SIDE]) {
  return (rowCrossed(board) || columnCrossed(board) || diagonalCrossed(board));
}

int inputMove(std::unordered_set<int> &us) {
  int num;
  std::cout << "Enter a move { ";
  printSet(us);
  std::cout << "}: ";
  auto numIt = us.find(num);
  while (1) {
    std::cin >> num;
    numIt = us.find(num);
    if (std::cin.fail() || numIt == us.end()) {
      std::cout << "Invalid Move: Enter a move { ";
      printSet(us);
      std::cout << "}: ";
    } else {
      numIt = us.erase(numIt);
      return num;
    }
  }
}

// A function to play Tic-Tac-Toe
void playTicTacToe(int playerNumber,
                   firebase::database::DatabaseReference &ref) {
  int whoseTurn = HUMAN;
  const std::map<int, char> playerMove{{0, 'O'}, {1, 'X'}};
  firebase::Future<void> fLastMove, fWhoseTurn;
  // Giving these default values with the intention of adding listeners to them
  fLastMove = ref.Child("lastMove").SetValue(0);
  fWhoseTurn = ref.Child("whoseTurn").SetValue(whoseTurn);
  WaitForCompletion(fLastMove, "setLastMove");
  WaitForCompletion(fWhoseTurn, "setWhoseTurn");

  // connectedUsersListener and whoseTurnListener listener is set up to
  // recognise when the desired players have connected & when turns alternate
  LogMessage("connectedUsersListener");
  ExpectValueListener *connectedUsersListener =
      new ExpectValueListener(COMPUTER);
  SampleValueListener *whoseTurnListener = new SampleValueListener();
  SampleValueListener *lastMoveListener = new SampleValueListener();

  ref.Child("connectedUsers").AddValueListener(connectedUsersListener);
  ref.Child("whoseTurn").AddValueListener(whoseTurnListener);
  ref.Child("lastMove").AddValueListener(lastMoveListener);

  // GP-TODO Add GameOver Bool and Listenere

  // A 3*3 Tic-Tac-Toe board for playing
  char board[SIDE][SIDE];
  int moves[] = {1, 2, 4, 5,
                 6, 7, 8, 9};  // GP-TODO - Make this populate based on side
  std::unordered_set<int> moveSet;
  int move;
  // Initialise the game
  initialise(board, moveSet);
  // Show the instructions before playing
  showInstructions();
  int x, y;

  // Initial Settup for HUMAN
  // HUMAN wait's for COMPUTER to join then completes their first move
  if (playerNumber == HUMAN) {
    while (!connectedUsersListener->got_value()) {
      ProcessEvents(1000);
    }
    move = inputMove(moveSet) - 1;
    x = move / SIDE;
    y = move % SIDE;
    board[x][y] = playerMove.at(HUMAN);
    printf("HUMAN has put a %c in cell %d\n", playerMove.at(HUMAN), move + 1);
    showBoard(board);
    fLastMove = ref.Child("lastMove").SetValue(move + 1);
    fWhoseTurn = ref.Child("whoseTurn").SetValue(COMPUTER);
    WaitForCompletion(fLastMove, "setLastMove");
    WaitForCompletion(fWhoseTurn, "setWhoseTurn");
    whoseTurn = (int)!playerNumber;
  }

  // Keep playing till the game is over or it is a draw
  while (!gameOver(board) && moveSet.size()) {
    LogMessage("WTL %lld,", whoseTurnListener->last_seen_value());
    if (whoseTurnListener->last_seen_value() == playerNumber) {
      LogMessage("Player %d: it is my turn!", playerNumber);
      int lastMove =
          lastMoveListener->last_seen_value().AsInt64().int64_value();
      auto moveIt = moveSet.find(lastMove);
      moveIt = moveSet.erase(moveIt);
      x = (lastMove - 1) / SIDE;
      y = (lastMove - 1) % SIDE;
      board[x][y] = playerMove.at((int)!playerNumber);
      printf("HUMAN has put a %c in cell %d\n",
             playerMove.at((int)!playerNumber), lastMove);
      showBoard(board);
      if (gameOver(board) || !moveSet.size()) {
        whoseTurn = playerNumber;
        break;
      }
      move = inputMove(moveSet) - 1;
      x = move / SIDE;
      y = move % SIDE;
      board[x][y] = playerMove.at((int)playerNumber);
      printf("HUMAN has put a %c in cell %d\n",
             playerMove.at((int)playerNumber), move + 1);
      showBoard(board);
      fLastMove = ref.Child("lastMove").SetValue(move + 1);
      fWhoseTurn = ref.Child("whoseTurn").SetValue((int)!playerNumber);
      WaitForCompletion(fLastMove, "setLastMove");
      WaitForCompletion(fWhoseTurn, "setWhoseTurn");
      whoseTurn = (int)!playerNumber;
    } else {
      // LogMessage("Player %d: Waiting for my turn", playerNumber);
      ProcessEvents(100);
    }
  }
  // If the game has drawn
  if (gameOver(board) == false && moveSet.size() == SIDE * SIDE)
    printf("It's a draw\n");
  else {
    // Declare the winner
    declareWinner(whoseTurn, playerNumber);
  }

  delete whoseTurnListener;
  whoseTurnListener = nullptr;
  delete connectedUsersListener;
  connectedUsersListener = nullptr;
  return;
}

extern "C" int common_main(int argc, const char *argv[]) {
  ::firebase::App *app;

#if defined(__ANDROID__)
  app = ::firebase::App::Create(GetJniEnv(), GetActivity());
#else
  app = ::firebase::App::Create();
#endif  // defined(__ANDROID__)

  LogMessage("Initialized Firebase App.");

  LogMessage("Initialize Firebase Auth and Firebase Database.");

  // Use ModuleInitializer to initialize both Auth and Database, ensuring no
  // dependencies are missing.
  ::firebase::database::Database *database = nullptr;
  ::firebase::auth::Auth *auth = nullptr;
  void *initialize_targets[] = {&auth, &database};

  const firebase::ModuleInitializer::InitializerFn initializers[] = {
      [](::firebase::App *app, void *data) {
        LogMessage("Attempt to initialize Firebase Auth.");
        void **targets = reinterpret_cast<void **>(data);
        ::firebase::InitResult result;
        *reinterpret_cast<::firebase::auth::Auth **>(targets[0]) =
            ::firebase::auth::Auth::GetAuth(app, &result);
        return result;
      },
      [](::firebase::App *app, void *data) {
        LogMessage("Attempt to initialize Firebase Database.");
        void **targets = reinterpret_cast<void **>(data);
        ::firebase::InitResult result;
        *reinterpret_cast<::firebase::database::Database **>(targets[1]) =
            ::firebase::database::Database::GetInstance(app, &result);
        return result;
      }};

  ::firebase::ModuleInitializer initializer;
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
    firebase::Future<firebase::auth::User *> sign_in_future =
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

  std::string saved_url;  // persists across connections

  // Create a unique child in the database that we can run our tests in.
  firebase::database::DatabaseReference ref;
  std::cout << "(1)Create Room, (0)Join Game: ";
  bool createdGame;
  std::string gameReferenceUrl;
  std::cin >> createdGame;
  if (createdGame) {
    ref = database->GetReference("test_app_data").PushChild();
    ref.Child("connectedUsers").SetValue(HUMAN);
    saved_url = ref.url();
    LogMessage("URL: %s", saved_url.c_str());
    playTicTacToe(HUMAN, ref);
  } else {
    std::cout << "Enter URL: ";
    std::cin >> gameReferenceUrl;
    ref = database->GetReferenceFromUrl(gameReferenceUrl.c_str());
    ref.Child("connectedUsers").SetValue(COMPUTER);
    playTicTacToe(COMPUTER, ref);
  }

  if (ref.is_valid()) {
    LogMessage("SUCCESS: Reference was validated on connection.");
  } else {
    LogMessage("ERROR: reference not valid.");
  }
  saved_url = ref.url();
  LogMessage("URL to join: %s", saved_url.c_str());

  LogMessage("Shutdown the Database library.");
  delete database;
  database = nullptr;

  LogMessage("Signing out from anonymous account.");
  auth->SignOut();
  LogMessage("Shutdown the Auth library.");
  delete auth;
  auth = nullptr;

  LogMessage("Shutdown Firebase App.");
  delete app;

  // Wait until the user wants to quit the app.
  while (!ProcessEvents(1000)) {
  }

  return 0;
}