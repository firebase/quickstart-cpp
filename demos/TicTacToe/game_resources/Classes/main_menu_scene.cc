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

#include "main_menu_scene.h"

#include <regex>
#include <string>

#include "cocos/ui/UIButton.h"
#include "cocos/ui/UITextField.h"
#include "cocos2d.h"
#include "firebase/auth.h"
#include "firebase/database.h"
#include "firebase/future.h"
#include "firebase/util.h"
#include "tic_tac_toe_scene.h"
#include "util.h"

using cocos2d::CallFunc;
using cocos2d::Color3B;
using cocos2d::Color4B;
using cocos2d::Color4F;
using cocos2d::DelayTime;
using cocos2d::Director;
using cocos2d::DrawNode;
using cocos2d::Event;
using cocos2d::EventListenerTouchOneByOne;
using cocos2d::Label;
using cocos2d::Menu;
using cocos2d::MenuItem;
using cocos2d::MenuItemSprite;
using cocos2d::RepeatForever;
using cocos2d::Scene;
using cocos2d::Sequence;
using cocos2d::Size;
using cocos2d::Sprite;
using cocos2d::TextFieldTTF;
using cocos2d::TextHAlignment;
using cocos2d::Touch;
using cocos2d::Vec2;
using cocos2d::ui::Button;
using cocos2d::ui::TextField;
using cocos2d::ui::Widget;
using firebase::App;
using firebase::InitResult;
using firebase::kFutureStatusComplete;
using firebase::ModuleInitializer;
using firebase::auth::Auth;
using firebase::auth::kAuthErrorNone;
using firebase::database::Database;
using std::array;
using std::to_string;

// States for button images.
static const enum kImageState { kNormal, kPressed };

// Panel image filenames.
static const char* kSignUpPanelImage = "sign_up_panel.png";
static const char* kGameMenuPanelImage = "game_menu_panel.png";
static const char* kAuthPanelImage = "auth_panel.png";
static const char* kLoginPanelImage = "login_panel.png";
static const char* kUserRecordPanelImage = "user_record_panel.png";

// Button image filenames.
static const array<char*, 2> kCreateGameButton = {"create_game.png",
                                                  "create_game_dark.png"};
static const array<char*, 2> kJoinButton = {"join_game.png",
                                            "join_game_dark.png"};
static const array<char*, 2> kLoginButton = {"login.png", "login_dark.png"};
static const array<char*, 2> kLogoutButton = {"logout.png", "logout_dark.png"};
static const array<char*, 2> kBackButton = {"leave.png", "leave_dark.png"};
static const array<char*, 2> kSignUpButton = {"sign_up.png",
                                              "sign_up_dark.png"};
static const array<char*, 2> kSkipButton = {"skip.png", "skip_dark.png"};
static const array<char*, 2> kLeaveAnonButton = {"leave_anon.png",
                                                 "leave_anon_dark.png"};

// Text box filenames.
static const char* kTextFieldOneImage = "text_field_grey.png";
static const char* kTextFieldTwoImage = "text_field_white.png";

static const char* kBackgroundImage = "background.png";
static const char* kLoadingBackgroundImage = "loading_background.png";

// Regex that will validate if the email entered is a valid email pattern.
const std::regex email_pattern("(\\w+)(\\.|_)?(\\w*)@(\\w+)(\\.(\\w+))+");

Scene* MainMenuScene::createScene() {
  // Builds a simple scene that uses the bottom left cordinate point as (0,0)
  // and can have sprites, labels and layers added onto it.
  auto scene = Scene::create();
  auto layer = MainMenuScene::create();
  scene->addChild(layer);

  return scene;
}

bool MainMenuScene::init() {
  if (!Layer::init()) {
    return false;
  }

  // Initializes the firebase features.
  this->InitializeFirebase();

  // Initializes the loading layer by setting the background that expires on a
  // delay.
  this->InitializeLoadingLayer();

  // Initializes the authentication layer by creating the background and placing
  // all required cocos2d components.
  this->InitializeAuthenticationLayer();

  // Initializes the login layer by creating the background and placing all
  // required cocos2d components.
  this->InitializeLoginLayer();

  // Initializes the login layer by creating the background and placing all
  // required cocos2d components.
  this->InitializeSignUpLayer();

  // Initializes the game menu layer by creating the background and placing all
  // required cocos2d components.
  this->InitializeGameMenuLayer();

  // Kicks off the updating game loop.
  this->scheduleUpdate();

  return true;
}

// Initialize the firebase auth and database while also ensuring no
// dependencies are missing.
void MainMenuScene::InitializeFirebase() {
  LogMessage("Initialize Firebase App.");
  firebase::App* app;
#if defined(_ANDROID_)
  app = firebase::App::Create(GetJniEnv(), GetActivity());
#else
  app = firebase::App::Create();
#endif  // defined(ANDROID)

  LogMessage("Initialize Firebase Auth and Firebase Database.");

  // Use ModuleInitializer to initialize both Auth and Database, ensuring no
  // dependencies are missing.
  database_ = nullptr;
  auth_ = nullptr;
  void* initialize_targets[] = {&auth_, &database_};

  const firebase::ModuleInitializer::InitializerFn initializers[] = {
      [](firebase::App* app, void* data) {
        LogMessage("Attempt to initialize Firebase Auth.");
        void** targets = reinterpret_cast<void**>(data);
        firebase::InitResult result;
        *reinterpret_cast<firebase::auth::Auth**>(targets[0]) =
            firebase::auth::Auth::GetAuth(app, &result);
        return result;
      },
      [](firebase::App* app, void* data) {
        LogMessage("Attempt to initialize Firebase Database.");
        void** targets = reinterpret_cast<void**>(data);
        firebase::InitResult result;
        *reinterpret_cast<firebase::database::Database**>(targets[1]) =
            firebase::database::Database::GetInstance(app, &result);
        return result;
      }};

  firebase::ModuleInitializer initializer;
  initializer.Initialize(app, initialize_targets, initializers,
                         sizeof(initializers) / sizeof(initializers[0]));

  WaitForCompletion(initializer.InitializeLastResult(), "Initialize");

  if (initializer.InitializeLastResult().error() != 0) {
    LogMessage("Failed to initialize Firebase libraries: %s",
               initializer.InitializeLastResult().error_message());
    ProcessEvents(2000);
  }
  LogMessage("Successfully initialized Firebase Auth and Firebase Database.");

  database_->set_persistence_enabled(true);
}

// 1. Adds the user record label (wins, loses & ties).
// 2. Creates the background for the node.
// 3. Adds the join and create button.
// 4. Adds the enter code text field and their event listeners.
// 5. Adds the logout button.
void MainMenuScene::InitializeGameMenuLayer() {
  // Creates the game menu background.
  game_menu_background_ = this->CreateBackground(kBackgroundImage);
  game_menu_background_->setVisible(false);
  this->addChild(game_menu_background_);

  // Creates and places the panel on the background.
  const auto game_menu_panel_origin = Vec2(300, 295);
  const auto game_menu_panel = Sprite::create(kGameMenuPanelImage);
  game_menu_panel->setPosition(game_menu_panel_origin);
  game_menu_background_->addChild(game_menu_panel, /*layer_index=*/10);

  // Creates the user record panel.
  const auto user_record_panel = Sprite::create(kUserRecordPanelImage);
  user_record_panel->setPosition(Vec2(405, 575));
  game_menu_background_->addChild(user_record_panel);

  // Label to display the user's wins.
  user_record_wins_ =
      Label::createWithTTF("", "fonts/GoogleSans-Regular.ttf", 28);
  user_record_wins_->setTextColor(Color4B::GRAY);
  user_record_wins_->setPosition(Vec2(88, 33));
  user_record_panel->addChild(user_record_wins_);

  // Label to display the user's losses.
  user_record_loses_ =
      Label::createWithTTF("", "fonts/GoogleSans-Regular.ttf", 28);
  user_record_loses_->setTextColor(Color4B::GRAY);
  user_record_loses_->setPosition(Vec2(180, 33));
  user_record_panel->addChild(user_record_loses_);

  // Label to display the user's ties.
  user_record_ties_ =
      Label::createWithTTF("", "fonts/GoogleSans-Regular.ttf", 28);
  user_record_ties_->setTextColor(Color4B::GRAY);
  user_record_ties_->setPosition(Vec2(280, 33));
  user_record_panel->addChild(user_record_ties_);

  // Creates the join_text_field.
  auto join_text_field_position = Vec2(394, 80);
  auto join_text_field_size = Size(180, 80);
  auto join_text_field =
      TextField::create("code", "fonts/GoogleSans-Regular.ttf", 48);
  join_text_field->setTextColor(Color4B::GRAY);
  join_text_field->setPosition(join_text_field_position);
  join_text_field->setTouchSize(join_text_field_size);
  join_text_field->setTouchAreaEnabled(true);
  join_text_field->setMaxLength(/*max_characters=*/4);
  join_text_field->setMaxLengthEnabled(true);
  game_menu_panel->addChild(join_text_field, /*layer_index=*/1);

  // Adds the event listener to handle the actions for the text field.
  join_text_field->addEventListener([this](Ref* sender,
                                           TextField::EventType type) {
    auto join_text_field = dynamic_cast<TextField*>(sender);
    string join_text_field_string = join_text_field->getString();
    // Transforms the letter casing to uppercase.
    std::transform(join_text_field_string.begin(), join_text_field_string.end(),
                   join_text_field_string.begin(), toupper);
    switch (type) {
      case TextField::EventType::ATTACH_WITH_IME:
        // Adds the repeated blinking cursor action.
        CreateBlinkingCursorAction(join_text_field);
        break;
      case TextField::EventType::DETACH_WITH_IME:
        // Stops the blinking cursor.
        join_text_field->stopAllActions();
        break;
      case TextField::EventType::INSERT_TEXT:
        join_text_field->setString(join_text_field_string);
        break;
      default:
        break;
    }
  });

  // Creates the background text box for the join_text_field_.
  const auto join_text_field_background = Sprite::create(kTextFieldTwoImage);
  join_text_field_background->setScale(1.47f);
  join_text_field_background->setPosition(join_text_field_position);
  game_menu_panel->addChild(join_text_field_background, /*layer_index=*/0);

  // Creates the create_button.
  auto create_button =
      Button::create(kCreateGameButton[kNormal], kCreateGameButton[kPressed]);
  create_button->setPosition(Vec2(271, 205));
  game_menu_panel->addChild(create_button);

  // Adds the event listener to swap scenes to the TicTacToe scene.
  create_button->addTouchEventListener(
      [this, join_text_field](Ref* sender, Widget::TouchEventType type) {
        switch (type) {
          case Widget::TouchEventType::ENDED:
            Director::getInstance()->pushScene(
                TicTacToe::createScene(std::string(), database_, user_uid_));
            join_text_field->setString("");
            state_ = kRunGameState;
            break;
          default:
            break;
        }
      });

  // Creates a sprite for the back button.
  back_button_ =
      Button::create(kLeaveAnonButton[kNormal], kLeaveAnonButton[kPressed]);
  back_button_->setPosition(Vec2(120, 575));
  game_menu_background_->addChild(back_button_);
  back_button_->setVisible(false);

  // Adds the event listener to change to the kAuthMenuState.
  back_button_->addTouchEventListener(
      [this, join_text_field](Ref* sender, Widget::TouchEventType type) {
        switch (type) {
          case Widget::TouchEventType::ENDED:
            user_uid_ = "";
            user_ = nullptr;
            user_result_.Release();
            user_record_wins_->setString("");
            user_record_loses_->setString("");
            user_record_ties_->setString("");
            back_button_->setVisible(false);
            state_ = kAuthMenuState;
            break;
          default:
            break;
        }
      });

  // Creates a sprite for the logout button.
  logout_button_ =
      Button::create(kLogoutButton[kNormal], kLogoutButton[kPressed]);
  logout_button_->setPosition(Vec2(120, 575));
  game_menu_background_->addChild(logout_button_);
  logout_button_->setVisible(false);

  // Adds the event listener to change to the kAuthMenuState.
  logout_button_->addTouchEventListener(
      [this, join_text_field](Ref* sender, Widget::TouchEventType type) {
        switch (type) {
          case Widget::TouchEventType::ENDED:
            user_uid_ = "";
            user_ = nullptr;
            user_result_.Release();
            user_record_wins_->setString("");
            user_record_loses_->setString("");
            user_record_ties_->setString("");
            logout_button_->setVisible(false);
            state_ = kAuthMenuState;
            break;
          default:
            break;
        }
      });

  // Creates a sprite for the join button and sets its position to the center
  // of the screen.
  auto join_button =
      Button::create(kJoinButton[kNormal], kJoinButton[kPressed]);
  join_button->setPosition(Vec2(148, 80));
  game_menu_panel->addChild(join_button);

  // Adds the event listener to handle touch actions for the join_button.
  join_button->addTouchEventListener(
      [this, join_text_field](Ref* sender, Widget::TouchEventType type) {
        // Get the string from join_text_field.
        const std::string join_text_field_string = join_text_field->getString();
        switch (type) {
          case Widget::TouchEventType::ENDED:
            if (join_text_field_string.length() == 4) {
              Director::getInstance()->pushScene(TicTacToe::createScene(
                  join_text_field_string, database_, user_uid_));
              state_ = kRunGameState;
              join_text_field->setString("");
            } else {
              join_text_field->setString("");
            }
            break;
          default:
            break;
        }
      });
}

// 1. Creates the background node.
// 2. Adds the error label and layer title label: sign up.
// 3. Adds the id and password text fields and their event listeners.
// 4. Adds the back and sign up button.
void MainMenuScene::InitializeSignUpLayer() {
  // Creates a background to add on all of the cocos2d components. The
  // visiblity of this node should match kSignUpState and only active this
  // layers event listeners.

  sign_up_background_ = this->CreateBackground(kBackgroundImage);
  sign_up_background_->setVisible(false);
  this->addChild(sign_up_background_);

  // Creates the sign up panel.
  const auto sign_up_panel_origin = Vec2(300, 325);
  const auto sign_up_panel = Sprite::create(kSignUpPanelImage);
  sign_up_panel->setPosition(sign_up_panel_origin);
  sign_up_background_->addChild(sign_up_panel, /*layer_index=*/10);

  // Label to output sign up errors.
  sign_up_error_label_ =
      Label::createWithTTF("", "fonts/GoogleSans-Regular.ttf", 20);
  sign_up_error_label_->setTextColor(Color4B(255, 82, 82, 240));
  sign_up_error_label_->setPosition(Vec2(255, 310));
  sign_up_panel->addChild(sign_up_error_label_);

  // Creates the sign_up_id_ text field.
  const auto id_font_size = 32;
  const auto id_position = Vec2(255, 260);
  const auto id_size = Size(450, id_font_size * 1.75);
  sign_up_id_ =
      TextField::create("Email", "fonts/GoogleSans-Regular.ttf", id_font_size);
  sign_up_id_->setTextColor(Color4B::GRAY);
  sign_up_id_->setPosition(id_position);
  sign_up_id_->setTouchAreaEnabled(true);
  sign_up_id_->setTouchSize(id_size);
  sign_up_panel->addChild(sign_up_id_, /*layer_index=*/1);

  // Creates the text box background for the sign_up_id_ text field.
  const auto id_background = Sprite::create(kTextFieldOneImage);
  id_background->setPosition(id_position);
  sign_up_panel->addChild(id_background, /*layer_index=*/0);

  // Adds the event listener to handle the actions for the sign_up_id_.
  sign_up_id_->addEventListener([this](Ref* sender, TextField::EventType type) {
    auto sign_up_id_ = dynamic_cast<TextField*>(sender);
    switch (type) {
      case TextField::EventType::ATTACH_WITH_IME:
        // Creates and runs the repeated blinking cursor action.
        CreateBlinkingCursorAction(sign_up_id_);
        break;
      case TextField::EventType::DETACH_WITH_IME:
        // Stops the blinking cursor.
        sign_up_id_->stopAllActions();
        break;
      default:
        break;
    }
  });

  // Creates the sign_up_password_ text field.
  const auto password_font_size = 32;
  const auto password_position = Vec2(255, 172);
  const auto password_size = Size(450, password_font_size * 1.75);
  sign_up_password_ = TextField::create(
      "Password", "fonts/GoogleSans-Regular.ttf", password_font_size);
  sign_up_password_->setTextColor(Color4B::GRAY);
  sign_up_password_->setPosition(password_position);
  sign_up_password_->setTouchAreaEnabled(true);
  sign_up_password_->setTouchSize(password_size);
  sign_up_password_->setPasswordEnabled(true);
  sign_up_panel->addChild(sign_up_password_, /*layer_index=*/1);

  // Creates the text box background for the sign_up_password_ text field.
  const auto password_background = Sprite::create(kTextFieldOneImage);
  password_background->setPosition(password_position);
  sign_up_panel->addChild(password_background, /*layer_index=*/0);

  // Adds the event listener to handle the actions for the sign_up_password_
  // text field.
  sign_up_password_->addEventListener(
      [this](Ref* sender, TextField::EventType type) {
        auto sign_up_password_ = dynamic_cast<TextField*>(sender);
        switch (type) {
          case TextField::EventType::ATTACH_WITH_IME:
            // Creates and runs the repeated blinking cursor action.
            CreateBlinkingCursorAction(sign_up_password_);
            break;
          case TextField::EventType::DETACH_WITH_IME:
            // Stops the blinking cursor.
            sign_up_password_->stopAllActions();
            break;
          default:
            break;
        }
      });

  // Creates the password_confirm text field.
  const auto password_confirm_font_size = 32;
  const auto password_confirm_position = Vec2(255, 85);
  const auto password_confirm_size =
      Size(450, password_confirm_font_size * 1.75);
  sign_up_password_confirm_ =
      TextField::create("Confirm password", "fonts/GoogleSans-Regular.ttf",
                        password_confirm_font_size);
  sign_up_password_confirm_->setTextColor(Color4B::GRAY);
  sign_up_password_confirm_->setPosition(password_confirm_position);
  sign_up_password_confirm_->setTouchAreaEnabled(true);
  sign_up_password_confirm_->setTouchSize(password_confirm_size);
  sign_up_password_confirm_->setPasswordEnabled(true);
  sign_up_panel->addChild(sign_up_password_confirm_, /*layer_index=*/1);

  // Creates the text box background for the sign_up_password_confirm_ text
  // field.
  const auto password_confirm_background = Sprite::create(kTextFieldOneImage);
  password_confirm_background->setPosition(password_confirm_position);
  sign_up_panel->addChild(password_confirm_background, /*layer_index=*/0);

  // Adds the event listener to handle the actions for the
  // sign_up_password_confirm text field.
  sign_up_password_confirm_->addEventListener(
      [this](Ref* sender, TextField::EventType type) {
        auto sign_up_password_confirm_ = dynamic_cast<TextField*>(sender);
        switch (type) {
          case TextField::EventType::ATTACH_WITH_IME:
            // Creates and runs the repeated blinking cursor action.
            CreateBlinkingCursorAction(sign_up_password_confirm_);
            break;
          case TextField::EventType::DETACH_WITH_IME:
            // Stops the blinking cursor.
            sign_up_password_confirm_->stopAllActions();
            break;
          default:
            break;
        }
      });

  // Creates the sign_up_button.
  auto sign_up_button =
      Button::create(kSignUpButton[kNormal], kSignUpButton[kPressed]);
  sign_up_button->setPosition(Vec2(255, 385));
  sign_up_panel->addChild(sign_up_button);

  // Adds the event listener to handle touch actions for the sign_up_button.
  sign_up_button->addTouchEventListener(
      [this](Ref* sender, Widget::TouchEventType type) {
        switch (type) {
          case Widget::TouchEventType::ENDED:
            // Validates the id and passwords are valid, then sets the
            // user_result_ future and swaps to kSignUpState.
            if (!std::regex_match(sign_up_id_->getString(), email_pattern)) {
              sign_up_error_label_->setString("invalid email address");
            } else if (sign_up_password_->getString().length() < 8) {
              sign_up_error_label_->setString(
                  "password must be at least 8 characters long");
            } else if (sign_up_password_->getString() !=
                       sign_up_password_confirm_->getString()) {
              sign_up_error_label_->setString("passwords do not match");
            } else {
              // Clears error label and sets user_result_ to the future created
              // user.
              sign_up_error_label_->setString("");
              user_result_ = auth_->CreateUserWithEmailAndPassword(
                  sign_up_id_->getString().c_str(),
                  sign_up_password_->getString().c_str());

              // Sets the state to kSignUpState.
              state_ = kSignUpState;
            }
            break;
          default:
            break;
        }
      });

  // Creates the return button.
  auto return_button =
      Button::create(kBackButton[kNormal], kBackButton[kPressed]);
  return_button->setScale(.3);
  return_button->setPosition(Size(50, 450));
  sign_up_panel->addChild(return_button);

  // Adds the event listener to return to kAuthMenuState.
  return_button->addTouchEventListener(
      [this](Ref* sender, Widget::TouchEventType type) {
        switch (type) {
          case Widget::TouchEventType::ENDED:
            // Clears all of the labels and text fields before swaping state.
            this->ClearAuthFields();
            state_ = kAuthMenuState;
            break;
          default:
            break;
        }
      });
}

// 1. Creates the background node.
// 2. Adds the error label and layer title label: login.
// 3. Adds the id and password text fields and their event listeners.
// 4. Adds the back and login button.
void MainMenuScene::InitializeLoginLayer() {
  // Creates the game menu background.
  login_background_ = this->CreateBackground(kBackgroundImage);
  login_background_->setVisible(false);
  this->addChild(login_background_);

  // Creates the login panel.
  const auto login_panel_origin = Vec2(300, 325);
  const auto login_panel = Sprite::create(kLoginPanelImage);
  login_panel->setPosition(login_panel_origin);
  login_background_->addChild(login_panel, /*layer_index=*/10);

  // Label to output login errors.
  login_error_label_ =
      Label::createWithTTF("", "fonts/GoogleSans-Regular.ttf", 24);
  login_error_label_->setTextColor(Color4B(255, 82, 82, 240));
  login_error_label_->setPosition(Vec2(255, 225));
  login_panel->addChild(login_error_label_);

  // Creating the login_id_ text field.
  const auto id_font_size = 32;
  const auto id_position = Vec2(255, 172);
  const auto id_size = Size(450, id_font_size * 1.75);
  login_id_ =
      TextField::create("Email", "fonts/GoogleSans-Regular.ttf", id_font_size);
  login_id_->setTextColor(Color4B::GRAY);
  login_id_->setPosition(id_position);
  login_id_->setTouchAreaEnabled(true);
  login_id_->setTouchSize(id_size);
  login_panel->addChild(login_id_, /*layer_index=*/1);

  // Creates the text box background for the login id text field.
  auto id_background = Sprite::create(kTextFieldOneImage);
  id_background->setPosition(id_position);
  login_panel->addChild(id_background, /*layer_index=*/0);

  // Adds the event listener to handle the actions for the login_id_.
  login_id_->addEventListener([this](Ref* sender, TextField::EventType type) {
    auto login_id_ = dynamic_cast<TextField*>(sender);
    switch (type) {
      case TextField::EventType::ATTACH_WITH_IME:
        // Creates and runs the repeated blinking cursor action.
        CreateBlinkingCursorAction(login_id_);
        break;
      case TextField::EventType::DETACH_WITH_IME:
        // Stops the blinking cursor.
        login_id_->stopAllActions();
        break;
      default:
        break;
    }
  });

  // Creates the login_password_ text field.
  const auto password_font_size = 32;
  const auto password_position = Vec2(255, 75);
  const auto password_size = Size(450, password_font_size * 1.75);
  login_password_ = TextField::create(
      "Password", "fonts/GoogleSans-Regular.ttf", password_font_size);
  login_password_->setTextColor(Color4B::GRAY);
  login_password_->setPosition(password_position);
  login_password_->setTouchAreaEnabled(true);
  login_password_->setTouchSize(password_size);
  login_password_->setPasswordEnabled(true);
  login_panel->addChild(login_password_, /*layer_index=*/1);

  // Creates the text box background for the login password text field.
  auto password_background = Sprite::create(kTextFieldOneImage);
  password_background->setPosition(password_position);
  login_panel->addChild(password_background, /*layer_index=*/0);

  // Adds the event listener to handle the actions for the login_password_ text
  // field.
  login_password_->addEventListener(
      [this](Ref* sender, TextField::EventType type) {
        auto login_password_ = dynamic_cast<TextField*>(sender);
        switch (type) {
          case TextField::EventType::ATTACH_WITH_IME:
            // Creates and runs the repeated blinking cursor action.
            CreateBlinkingCursorAction(login_password_);
            break;
          case TextField::EventType::DETACH_WITH_IME:
            // Stops the blinking cursor.
            login_password_->stopAllActions();
            break;
          default:
            break;
        }
      });

  // Creates the login_button.
  auto login_button =
      Button::create(kLoginButton[kNormal], kLoginButton[kPressed]);
  login_button->setPosition(Size(255, 300));
  login_panel->addChild(login_button);

  // Adds the event listener to handle touch actions for the login_button.
  login_button->addTouchEventListener(
      [this](Ref* sender, Widget::TouchEventType type) {
        switch (type) {
          case Widget::TouchEventType::ENDED:
            // Validates the id and passwords are valid, then sets the
            // user_result_ future.
            if (!std::regex_match(login_id_->getString(), email_pattern)) {
              login_error_label_->setString("invalid email address");
            } else if (login_password_->getString().length() < 8) {
              login_error_label_->setString(
                  "password must be at least 8 characters long");
            } else {
              // Clears error label and sets user_result_ to the future existing
              // user.
              login_error_label_->setString("");
              user_result_ = auth_->SignInWithEmailAndPassword(
                  login_id_->getString().c_str(),
                  login_password_->getString().c_str());
            }
            break;
          default:
            break;
        }
      });

  // Creates the return button.
  auto return_button =
      Button::create(kBackButton[kNormal], kBackButton[kPressed]);
  return_button->setScale(.3);
  return_button->setPosition(Size(50, 375));
  login_panel->addChild(return_button);

  // Adds the event listener to return to kAuthMenuState.
  return_button->addTouchEventListener(
      [this](Ref* sender, Widget::TouchEventType type) {
        switch (type) {
          case Widget::TouchEventType::ENDED:
            // Clears all of the labels and text fields before swaping state.
            this->ClearAuthFields();
            state_ = kAuthMenuState;
            break;
          default:
            break;
        }
      });
}

// Creates and places the loading background. Creates a action sequence to delay
// then swap to the authentication state.
void MainMenuScene::InitializeLoadingLayer() {
  // Creates the delay action.
  auto loading_delay = DelayTime::create(/*delay_durration*/ 2.0f);

  // Creates a callback function to swap state to kAuthMenuState.
  auto SwapToAuthState =
      CallFunc::create([this]() { state_ = kAuthMenuState; });

  // Runs the sequence that will delay followed by the swap state callback
  // function.
  this->runAction(Sequence::create(loading_delay, SwapToAuthState, NULL));

  // Creates the loading background sprite.
  loading_background_ = this->CreateBackground(kLoadingBackgroundImage);
  loading_background_->setContentSize(Size(600, 641));
  this->addChild(loading_background_);
}

// 1. Creates the background node.
// 2. Adds the layer title label: authentication.
// 3. Adds the id and password text fields and their event listeners.
// 4. Adds the back and login button.
void MainMenuScene::InitializeAuthenticationLayer() {
  // Creates the auth_background.
  auth_background_ = this->CreateBackground(kBackgroundImage);
  auth_background_->setVisible(false);
  this->addChild(auth_background_);

  // Creates the auth panel.
  const auto auth_panel_origin = Vec2(300, 315);
  const auto auth_panel = Sprite::create(kAuthPanelImage);
  auth_panel->setPosition(auth_panel_origin);
  auth_background_->addChild(auth_panel, /*layer_index=*/10);

  // Creates three buttons for the menu items (login,sign up, and anonymous sign
  // in).
  // For each menu item button, creates a normal and selected version and attach
  // the touch listener.

  // Creates the sign up menu item.
  const auto sign_up_normal_item = Sprite::create(kSignUpButton[kNormal]);
  const auto sign_up_selected = Sprite::create(kSignUpButton[kPressed]);

  auto sign_up_item = MenuItemSprite::create(
      sign_up_normal_item, sign_up_selected, [this](Ref* sender) {
        auto node = dynamic_cast<Node*>(sender);
        if (node != nullptr) {
          state_ = kSignUpState;
        }
      });
  sign_up_item->setTag(0);

  // Creates the login menu item.
  const auto login_normal_item = Sprite::create(kLoginButton[kNormal]);
  const auto login_selected_item = Sprite::create(kLoginButton[kPressed]);

  auto login_item = MenuItemSprite::create(
      login_normal_item, login_selected_item, [this](Ref* sender) {
        auto node = dynamic_cast<Node*>(sender);
        if (node != nullptr) {
          state_ = kLoginState;
        }
      });
  login_item->setTag(1);

  // Creates the skip login menu item.
  const auto skip_login_normal_item = Sprite::create(kSkipButton[kNormal]);
  const auto skip_login_selected_item = Sprite::create(kSkipButton[kPressed]);

  auto skip_login_item = MenuItemSprite::create(
      skip_login_normal_item, skip_login_selected_item, [this](Ref* sender) {
        auto node = dynamic_cast<Node*>(sender);
        if (node != nullptr) {
          user_result_ = auth_->SignInAnonymously();
          state_ = kSkipLoginState;
        }
      });
  skip_login_item->setTag(2);

  // Combines the individual items to create the menu.
  cocos2d::Vector<MenuItem*> menuItems = {sign_up_item, login_item,
                                          skip_login_item};
  auto menu = Menu::createWithArray(menuItems);
  menu->setPosition(Size(200, 245));
  menu->setContentSize(Size(100, 200));
  menu->alignItemsVerticallyWithPadding(30.0f);
  auth_panel->addChild(menu);
}

// Gets the user record variables to reflect what is in the database.
void MainMenuScene::GetUserRecord() {
  ref_ = database_->GetReference("users").Child(user_uid_);
  auto future_wins = ref_.Child("wins").GetValue();
  auto future_loses = ref_.Child("loses").GetValue();
  auto future_ties = ref_.Child("ties").GetValue();
  WaitForCompletion(future_wins, "getUserWinsData");
  WaitForCompletion(future_loses, "getUserLosesData");
  WaitForCompletion(future_ties, "getUserTiesData");
  user_wins_ = future_wins.result()->value().int64_value();
  user_loses_ = future_loses.result()->value().int64_value();
  user_ties_ = future_ties.result()->value().int64_value();
}

// Sets the user record variables to reflect the user record local variables.
void MainMenuScene::SetUserRecord() {
  ref_ = database_->GetReference("users").Child(user_uid_);
  auto future_wins = ref_.Child("wins").SetValue(user_wins_);
  auto future_loses = ref_.Child("loses").SetValue(user_loses_);
  auto future_ties = ref_.Child("ties").SetValue(user_ties_);
  WaitForCompletion(future_wins, "setUserWinsData");
  WaitForCompletion(future_loses, "setUserLosesData");
  WaitForCompletion(future_ties, "setUserTiesData");
}

// Clears the user record.
void MainMenuScene::ClearUserRecord() {
  user_wins_ = 0;
  user_loses_ = 0;
  user_ties_ = 0;
}

// Displays the user record.
void MainMenuScene::DisplayUserRecord() {
  user_record_wins_->setString(to_string(user_wins_));
  user_record_loses_->setString(to_string(user_loses_));
  user_record_ties_->setString(to_string(user_ties_));
}

// Overriding the onEnter method to update the user_record on reenter.
void MainMenuScene::onEnter() {
  // If the scene is entering from the game, UpdateUserRecords() and change
  // state_ back to kGameMenuState.
  if (state_ == kRunGameState) {
    this->GetUserRecord();
    this->DisplayUserRecord();
    state_ = kGameMenuState;
  }
  Layer::onEnter();
}

// Clears all of the labels and text fields on the login and sign up layers.
void MainMenuScene::ClearAuthFields() {
  // Clears the login components.
  login_id_->setString("");
  login_password_->setString("");
  login_error_label_->setString("");

  // Clears the sign up components.
  sign_up_id_->setString("");
  sign_up_password_->setString("");
  sign_up_password_confirm_->setString("");
  sign_up_error_label_->setString("");
}

// Updates every frame:
//
// switch (state_)
// (0) kInitializingState: swaps to (1).
// (1) kAuthMenuState: makes the auth_background_ visable.
// (2) kGameMenuState: makes the game_menu_background_ invisable.
// (3) kSkipLoginState: waits for anonymous sign in then swaps to (2).
// (4) kSignUpState: waits for sign up future completion,
//     updates user variables, and swaps to (2).
// (5) kLoginState: waits for login future completion,
//     updates user variables, and swaps to (2).
// (6) kRunGameState: waits for director to pop the TicTacToeScene.
void MainMenuScene::update(float /*delta*/) {
  switch (state_) {
    case kInitializingState:
      state_ = UpdateInitialize();
      break;
    case kAuthMenuState:
      state_ = UpdateAuthentication();
      break;
    case kGameMenuState:
      state_ = UpdateGameMenu();
      break;
    case kSkipLoginState:
      state_ = UpdateSkipLogin();
      break;
    case kSignUpState:
      state_ = UpdateSignUp();
      break;
    case kLoginState:
      state_ = UpdateLogin();
      break;
    case kRunGameState:
      state_ = UpdateRunGame();
      break;
    default:
      assert(0);
  }
}

// Returns kInitializingState. Waits for the delay action sequence to callback
// SwaptoAuthState() to set state_ = kAuthMenuState.
MainMenuScene::kSceneState MainMenuScene::UpdateInitialize() {
  this->UpdateLayer(state_);
  return kInitializingState;
}

// Updates the layer and returns the kAuthMenuState.
MainMenuScene::kSceneState MainMenuScene::UpdateAuthentication() {
  this->UpdateLayer(state_);
  return kAuthMenuState;
}

// Updates the layer and stays in this state until user_result_ completes.
// Updates the user variables if the user_result_ is valid. Updates the error
// message and returns back to kLoginState if the future user_result_ errored.
MainMenuScene::kSceneState MainMenuScene::UpdateLogin() {
  this->UpdateLayer(state_);
  if (user_result_.status() == firebase::kFutureStatusComplete) {
    if (user_result_.error() == firebase::auth::kAuthErrorNone) {
      // Updates the user to refect the uid and record (wins,losses and ties)
      // stored for the user in the database.
      user_ = *user_result_.result();
      user_uid_ = user_->uid();
      this->ClearAuthFields();
      this->GetUserRecord();
      this->DisplayUserRecord();

      // Shows the logout button because the user logged in.
      logout_button_->setVisible(true);

      return kGameMenuState;
    } else {
      // Changes login_error_label_ to display the user_result_ future
      // errored.
      login_error_label_->setString("invalid credentials");
      user_result_.Release();
      return kLoginState;
    }
  } else {
    return kLoginState;
  }
}

// Updates the layer and stays in this state until user_result_ completes.
// Initializes the user if the user_result_ is valid. Updates the error
// message and returns back to kSignUpState if the future user_result_
// errored.
MainMenuScene::kSceneState MainMenuScene::UpdateSignUp() {
  this->UpdateLayer(state_);
  if (user_result_.status() == firebase::kFutureStatusComplete) {
    if (user_result_.error() == firebase::auth::kAuthErrorNone) {
      // Initializes user variables and stores them in the database.
      user_ = *user_result_.result();
      user_uid_ = GenerateUid(10);
      this->ClearUserRecord();
      this->DisplayUserRecord();

      // Shows the logout button because the user signed up.
      logout_button_->setVisible(true);

      return kGameMenuState;
    } else {
      // Changes sign_up_error_label_ to display the user_result_ future
      // errored.
      sign_up_error_label_->setString("sign up failed");
      user_result_.Release();
      return kSignUpState;
    }
  } else {
    return kSignUpState;
  }
}

// Updates the layer and stays in this state until user_result_ completes.
// Initializes the user if the user_result_ is valid. Otherwise, return back
// to kAuthMenuState.
MainMenuScene::kSceneState MainMenuScene::UpdateSkipLogin() {
  if (user_result_.status() == firebase::kFutureStatusComplete) {
    if (user_result_.error() == firebase::auth::kAuthErrorNone) {
      // Initializes user variables and stores them in the database.
      user_ = *user_result_.result();
      user_uid_ = GenerateUid(10);
      this->ClearUserRecord();
      this->SetUserRecord();
      this->DisplayUserRecord();

      // Shows the back button because the user skipped login.
      back_button_->setVisible(true);

      return kGameMenuState;
    } else {
      CCLOG("Error skipping login.");
      return kAuthMenuState;
    }

  } else {
    return kSkipLoginState;
  }
}

// Updates the layer and returns kGameMenuState.
MainMenuScene::kSceneState MainMenuScene::UpdateGameMenu() {
  this->UpdateLayer(state_);
  return kGameMenuState;
}

// Continues to return that you are in the kRunGameState.
MainMenuScene::kSceneState MainMenuScene::UpdateRunGame() {
  return kRunGameState;
}

// Returns a repeating action that toggles the cursor of the text field passed
// in based on the toggle_delay.
void MainMenuScene::CreateBlinkingCursorAction(
    cocos2d::ui::TextField* text_field) {
  // Creates a callable function that shows the cursor and sets the cursor
  // character.
  const auto show_cursor = CallFunc::create([text_field]() {
    text_field->setCursorEnabled(true);
    text_field->setCursorChar('|');
  });

  // Creates a callable function that hides the cursor character.
  const auto hide_cursor =
      CallFunc::create([text_field]() { text_field->setCursorChar(' '); });

  // Creates a delay action.
  const cocos2d::DelayTime* delay = DelayTime::create(/*delay_durration=*/0.3f);

  // Aligns the sequence of actions to create a blinking cursor.
  auto blink_cursor_action =
      Sequence::create(show_cursor, delay, hide_cursor, delay, nullptr);

  // Creates a forever repeating action based on the blink_cursor_action.
  text_field->runAction(RepeatForever::create(blink_cursor_action));
}

// Creates a background the same size as the window and places it to cover the
// entire window.
Sprite* MainMenuScene::CreateBackground(const string& background_image) {
  const auto window_size = Director::getInstance()->getWinSize();
  const auto background = Sprite::create(background_image);
  background->setContentSize(window_size);
  background->setAnchorPoint(Vec2(0, 0));
  return background;
}

// Updates the auth_,login_, sign_up_, and game_menu_ layer based on state.
void MainMenuScene::UpdateLayer(MainMenuScene::kSceneState state) {
  auth_background_->setVisible(state == kAuthMenuState);
  login_background_->setVisible(state == kLoginState);
  sign_up_background_->setVisible(state == kSignUpState);
  game_menu_background_->setVisible(state == kGameMenuState);
  loading_background_->setVisible(state == kInitializingState);
}