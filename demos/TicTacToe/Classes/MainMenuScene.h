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

#ifndef TICTACTOE_DEMO_CLASSES_MAINMENU_SCENE_H_
#define TICTACTOE_DEMO_CLASSES_MAINMENU_SCENE_H_

#include "cocos2d.h"
#include "firebase/auth.h"
#include "firebase/database.h"
#include "firebase/future.h"
#include "firebase/util.h"
#include "ui/CocosGUI.h"
using std::to_string;

void LogMessage(const char*, ...);
void ProcessEvents(int);
void WaitForCompletion(const firebase::FutureBase&, const char*);
std::string GenerateUid(std::size_t);

class MainMenuScene : public cocos2d::Layer, public cocos2d::TextFieldDelegate {
 public:
  // Defines the state the class is currently in, which updates the sprites in
  // the MainMenuScene::update(float) method.
  enum kSceneState {
    kAuthState,
    kGameMenuState,
    kWaitingAnonymousState,
    kWaitingSignUpState,
    kWaitingLoginState,
    kWaitingGameOutcome
  };

  // Build a simple scene that uses the bottom left cordinate point as (0,0)
  // and can have sprites, labels and nodes added onto it.
  static cocos2d::Scene* createScene();
  void MainMenuScene::update(float) override;
  void MainMenuScene::onEnter() override;

  // Updates the user record (wins,loses and ties) and displays it to the
  // screen.
  void MainMenuScene::UpdateUserRecord();

  // Initializes the user record (wins,loses and ties) and displays it to the
  // screen.
  void MainMenuScene::InitializeUserRecord();
  void MainMenuScene::InitializeFirebase();

  // Initialize the instance of a Node and returns a boolean based on if it was
  // successful in doing so.
  virtual bool init();
  CREATE_FUNC(MainMenuScene);

  // Creates node to be used as a background for the authentication menu.
  cocos2d::DrawNode* auth_background;
  cocos2d::Node auth_node;

  // Labels and textfields for the authentication menu.
  cocos2d::Label* invalid_login_label;
  cocos2d::Label* user_record_label;
  cocos2d::TextFieldTTF* email_text_field;
  cocos2d::TextFieldTTF* password_text_field;

  // Variable to track the current state and previous state to check against to
  // see if the state change.
  kSceneState current_state = kAuthState;
  kSceneState previous_state = kAuthState;

  // User record varibales that are stored in firebase database.
  int user_wins;
  int user_loses;
  int user_ties;

  std::string user_uid;
  firebase::auth::User* user;
  firebase::Future<firebase::auth::User*> user_result;
  firebase::database::Database* database;
  firebase::auth::Auth* auth;
  firebase::database::DatabaseReference ref;
};

#endif  // TICTACTOE_DEMO_CLASSES_MAINMENU_SCENE_H_
