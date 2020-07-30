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

#include "cocos/ui/UITextField.h"
#include "cocos2d.h"
#include "firebase/auth.h"
#include "firebase/database.h"
#include "firebase/future.h"

using cocos2d::Color4F;
using cocos2d::DrawNode;
using cocos2d::Label;
using cocos2d::Size;
using cocos2d::TextFieldTTF;
using cocos2d::Vec2;
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

 private:
  // Defines the state the class is currently in, which updates the sprites in
  // the MainMenuScene::update(float) method.
  enum kSceneState {
    kInitializingState,
    kAuthMenuState,
    kLoginState,
    kSignUpState,
    kGameMenuState,
    kSkipLoginState,
    kRunGameState
  };

  // Creates and runs an endless blinking cursor action for the textfield passed
  // in.
  void CreateBlinkingCursorAction(cocos2d::ui::TextField*);

  // Updates the scene to show the active layer based on state.
  void UpdateLayer(MainMenuScene::kSceneState);

  // The game loop method for this layer which runs every frame once scheduled
  // using this->scheduleUpdate(). Acts as the state manager for this scene.
  void update(float) override;

  // Update methods to corresponding to each kSceneState.
  kSceneState UpdateAuthentication();
  kSceneState UpdateGameMenu();
  kSceneState UpdateInitialize();
  kSceneState UpdateLogin();
  kSceneState UpdateSignUp();
  kSceneState UpdateSkipLogin();
  kSceneState UpdateRunGame();

  // If the scene is re-entered from TicTacToeScene, then call
  // UpdateUserRecord() and swap state_ to kGameMenuState.
  void onEnter() override;

  // Updates the user record (wins,loses and ties) and displays it to the
  // screen.
  void UpdateUserRecord();

  // Initializes the user record (wins,loses and ties) and displays it to the
  // screen.
  void InitializeUserRecord();

  // Initializes the loading layer which includes a background loading image and
  // state swap delay action.
  void InitializeLoadingLayer();

  // Initializes the game menu layer which includes the background, buttons
  // and labels related to setting up the game menu.
  void InitializeGameMenuLayer();

  // Initializes the authentication layer which includes the background, buttons
  // and labels related to authenticating the user.
  void InitializeAuthenticationLayer();

  // Initializes the sign up layer which includes the background, buttons
  // and labels related to signing up the user.
  void InitializeSignUpLayer();

  // Initializes the login layer which includes the background, buttons
  // and labels.
  void InitializeLoginLayer();

  // Clears the labels and text fields for all authentication layers.
  void ClearAuthFields();

  // Creates a rectangle from the size, origin, border_color, background_color,
  // and border_thickness.
  DrawNode* CreateRectangle(Size size, Vec2 origin,
                            Color4F background_color = Color4F(0, 0, 0, 0),
                            Color4F border_color = Color4F::WHITE,
                            int border_thickness = 1);

  // Initializes the the firebase app, auth, and database.
  void InitializeFirebase();
  // Initializes the instance of a Node and returns a boolean based on if it was
  // successful in doing so.
  bool init() override;
  CREATE_FUNC(MainMenuScene);

  // Node to be used as a background for the authentication menu.
  cocos2d::DrawNode* auth_background_;

  // Node to be used as a background for the login menu.
  cocos2d::DrawNode* login_background_;

  // Node to be used as a background for the sign-up menu.
  cocos2d::DrawNode* sign_up_background_;

  // Node to be used as a background for the loading layer.
  cocos2d::Sprite* loading_background_;

  // Labels and textfields for the authentication menu.
  Label* login_error_label_;
  Label* sign_up_error_label_;
  Label* user_record_label_;

  // Cocos2d components for the login layer.
  cocos2d::ui::TextField* login_id_;
  cocos2d::ui::TextField* login_password_;

  // Cocos2d components for the sign up layer.
  cocos2d::ui::TextField* sign_up_id_;
  cocos2d::ui::TextField* sign_up_password_;
  cocos2d::ui::TextField* sign_up_password_confirm_;

  kSceneState state_ = kInitializingState;

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