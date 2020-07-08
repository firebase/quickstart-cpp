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

#include <string>

#include "cocos2d.h"
#include "firebase/auth.h"
#include "firebase/database.h"
#include "firebase/future.h"

using cocos2d::Label;
using cocos2d::TextFieldTTF;
using firebase::Future;
using firebase::auth::Auth;
using firebase::auth::User;
using firebase::database::Database;
using firebase::database::DatabaseReference;
using std::string;

class MainMenuScene : public cocos2d::Layer, public cocos2d::TextFieldDelegate {
 public:
  // Build a simple scene that uses the bottom left cordinate point as (0,0)
  // and can have sprites, labels and nodes added onto it.
  static cocos2d::Scene* createScene();

  // The game loop method for this layer which runs every frame once scheduled
  // using this->scheduleUpdate(). Acts as the state manager for this scene.
  void MainMenuScene::update(float) override;

  // If the scene is re-entered from TicTacToeScene, then call
  // UpdateUserRecord() and swap current_state_ to kGameMenuState.
  void MainMenuScene::onEnter() override;

  // Initializes the instance of a Node and returns a boolean based on if it was
  // successful in doing so.
  bool init() override;
  CREATE_FUNC(MainMenuScene);

 private:
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

  // Updates the user record (wins,loses and ties) and displays it to the
  // screen.
  void MainMenuScene::UpdateUserRecord();

  // Initializes the user record (wins,loses and ties) and displays it to the
  // screen.
  void MainMenuScene::InitializeUserRecord();
  // Initializes the the firebase app, auth, and database.
  void MainMenuScene::InitializeFirebase();

  // Node to be used as a background for the authentication menu.
  cocos2d::DrawNode* auth_background_;

  // Labels and textfields for the authentication menu.
  Label* invalid_login_label_;
  Label* user_record_label_;
  TextFieldTTF* email_text_field_;
  TextFieldTTF* password_text_field_;

  // Variable to track the current state and previous state to check against to
  // see if the state changed.
  kSceneState current_state_ = kAuthState;
  kSceneState previous_state_ = kAuthState;

  // User record variabales that are stored in firebase database.
  int user_wins_;
  int user_loses_;
  int user_ties_;

  string user_uid_;
  Auth* auth_;
  User* user_;
  Future<User*> user_result_;
  Database* database_;
  DatabaseReference ref_;
};

#endif  // TICTACTOE_DEMO_CLASSES_MAINMENU_SCENE_H_