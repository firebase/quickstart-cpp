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
using std::to_string;

static const char* kCreateGameImage = "create_game.png";
static const char* kTextFieldBorderImage = "text_field_border.png";
static const char* kJoinButtonImage = "join_game.png";
static const char* kLoginButtonImage = "login.png";
static const char* kLogoutButtonImage = "logout.png";
static const char* kSignUpButtonImage = "sign_up.png";

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
  // Label to display the users record.
  user_record_label_ =
      Label::createWithTTF("", "fonts/GoogleSans-Regular.ttf", 24);
  user_record_label_->setAlignment(TextHAlignment::RIGHT);
  user_record_label_->setTextColor(Color4B::WHITE);
  user_record_label_->setPosition(Vec2(500, 600));
  this->addChild(user_record_label_);

  // Creates the join_text_field.
  auto join_text_field_position = Size(480, 95);
  auto join_text_field_size = Size(180, 80);
  auto join_text_field =
      TextField::create("code", "fonts/GoogleSans-Regular.ttf", 48);
  join_text_field->setPosition(join_text_field_position);
  join_text_field->setTouchSize(join_text_field_size);
  join_text_field->setTouchAreaEnabled(true);
  join_text_field->setMaxLength(/*max_characters=*/4);
  join_text_field->setMaxLengthEnabled(true);
  this->addChild(join_text_field);

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

  // Creates the border for the join_text_field_.
  const auto join_text_field_border =
      CreateRectangle(join_text_field_size, join_text_field_position,
                      Color4F(0, 0, 0, 0), Color4F::WHITE);
  this->addChild(join_text_field_border);

  // Creates the create_button.
  auto create_button = Button::create(kCreateGameImage);
  create_button->setPosition(Vec2(300, 300));
  this->addChild(create_button);

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

  // Creates a sprite for the logout button and sets its position to the
  auto logout_button = Button::create(kLogoutButtonImage);
  logout_button->setPosition(Vec2(75, 575));
  logout_button->setScale(.4);
  this->addChild(logout_button);

  // Adds the event listener to change to the kAuthMenuState.
  logout_button->addTouchEventListener(
      [this, join_text_field](Ref* sender, Widget::TouchEventType type) {
        switch (type) {
          case Widget::TouchEventType::ENDED:
            user_uid_ = "";
            user_ = nullptr;
            user_result_.Release();
            user_record_label_->setString("");
            state_ = kAuthMenuState;
            break;
          default:
            break;
        }
      });

  // Creates a sprite for the join button and sets its position to the center
  // of the screen.
  auto join_button = Button::create(kJoinButtonImage, kJoinButtonImage);
  join_button->setPosition(Vec2(25, 50));
  join_button->setAnchorPoint(Vec2(0, 0));
  join_button->setScale(1.3f);
  this->addChild(join_button);

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

  // Creates and places the sign_up_background_.
  const auto sign_up_background_size = Size(500, 450);
  const auto sign_up_background_origin = Size(300, 300);
  sign_up_background_ =
      this->CreateRectangle(sign_up_background_size, sign_up_background_origin,
                            /*background_color=*/Color4F::BLACK);
  this->addChild(sign_up_background_, /*layer_index=*/10);

  // Creates the layer title label: sign up.
  auto layer_title =
      Label::createWithTTF("sign up", "fonts/GoogleSans-Regular.ttf", 48);
  layer_title->setAnchorPoint(Vec2(.5, .5));
  layer_title->setPosition(Vec2(300, 475));
  sign_up_background_->addChild(layer_title);

  // Label to output sign up errors.
  sign_up_error_label_ =
      Label::createWithTTF("", "fonts/GoogleSans-Regular.ttf", 24);
  sign_up_error_label_->setTextColor(Color4B::RED);
  sign_up_error_label_->setPosition(Vec2(300, 425));
  sign_up_background_->addChild(sign_up_error_label_);

  // Creates the sign_up_id_.
  const auto id_font_size = 28;
  const auto id_position = Size(300, 375);
  const auto id_size = Size(450, id_font_size * 1.75);
  sign_up_id_ =
      TextField::create("email", "fonts/GoogleSans-Regular.ttf", id_font_size);
  sign_up_id_->setPosition(id_position);
  sign_up_id_->setTouchAreaEnabled(true);
  sign_up_id_->setTouchSize(id_size);
  sign_up_background_->addChild(sign_up_id_);

  // Creates the border for the sign_up_id_ text field.
  auto id_border = this->CreateRectangle(id_size, id_position);
  sign_up_background_->addChild(id_border);

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

  // Creates the sign_up_password_.
  const auto password_font_size = 28;
  const auto password_position = Size(300, 300);
  const auto password_size = Size(450, password_font_size * 1.75);
  sign_up_password_ = TextField::create(
      "password", "fonts/GoogleSans-Regular.ttf", password_font_size);
  sign_up_password_->setPosition(password_position);
  sign_up_password_->setTouchAreaEnabled(true);
  sign_up_password_->setTouchSize(password_size);
  sign_up_password_->setPasswordEnabled(true);
  sign_up_background_->addChild(sign_up_password_);

  // Creates the border for the sign_up_password_ text field.
  auto password_border =
      this->CreateRectangle(password_size, password_position);
  sign_up_background_->addChild(password_border);

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

  // Creates the password_confirm text_field.
  const auto password_confirm_font_size = 28;
  const auto password_confirm_position = Size(300, 225);
  const auto password_confirm_size =
      Size(450, password_confirm_font_size * 1.75);
  sign_up_password_confirm_ =
      TextField::create("confirm password", "fonts/GoogleSans-Regular.ttf",
                        password_confirm_font_size);
  sign_up_password_confirm_->setPosition(password_confirm_position);
  sign_up_password_confirm_->setTouchAreaEnabled(true);
  sign_up_password_confirm_->setTouchSize(password_confirm_size);
  sign_up_password_confirm_->setPasswordEnabled(true);
  sign_up_background_->addChild(sign_up_password_confirm_);

  // Creates the border for the sign_up_password_confirm_ text field.
  auto password_confirm_border =
      this->CreateRectangle(password_confirm_size, password_confirm_position);
  sign_up_background_->addChild(password_confirm_border);

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
  auto sign_up_button = Button::create(kSignUpButtonImage, kLoginButtonImage);
  sign_up_button->setScale(.5);
  sign_up_button->setPosition(Size(300, 130));
  sign_up_background_->addChild(sign_up_button);

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

  // Creates the back_button.
  auto back_button = Button::create(kJoinButtonImage, kSignUpButtonImage);
  back_button->setScale(.5);
  back_button->setPosition(Size(130, 500));
  sign_up_background_->addChild(back_button);

  // Adds the event listener to swap back to kAuthMenuState.
  back_button->addTouchEventListener(
      [this](Ref* sender, Widget::TouchEventType type) {
        switch (type) {
          case Widget::TouchEventType::ENDED:
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
  // Creates a background to add on all of the cocos2d components. The
  // visiblity of this node should match kLoginState and only active this
  // layers event listeners.

  // Creates and places the login_background_.
  const auto login_background_size = Size(500, 450);
  const auto login_background_origin = Size(300, 300);
  login_background_ =
      this->CreateRectangle(login_background_size, login_background_origin,
                            /*background_color=*/Color4F::BLACK);
  this->addChild(login_background_, /*layer_index=*/10);

  // Creates the layer title label: login.
  auto layer_title =
      Label::createWithTTF("Login", "fonts/GoogleSans-Regular.ttf", 48);
  layer_title->setAnchorPoint(Vec2(.5, .5));
  layer_title->setPosition(Vec2(300, 475));
  login_background_->addChild(layer_title);

  // Label to output login errors.
  login_error_label_ =
      Label::createWithTTF("", "fonts/GoogleSans-Regular.ttf", 24);
  login_error_label_->setTextColor(Color4B::RED);
  login_error_label_->setPosition(Vec2(300, 425));
  login_background_->addChild(login_error_label_);

  // Creating the login_id_.
  const auto id_font_size = 28;
  const auto id_position = Size(300, 375);
  const auto id_size = Size(450, id_font_size * 1.75);
  login_id_ =
      TextField::create("email", "fonts/GoogleSans-Regular.ttf", id_font_size);
  login_id_->setPosition(id_position);
  login_id_->setTouchAreaEnabled(true);
  login_id_->setTouchSize(id_size);
  login_background_->addChild(login_id_);

  // Creates the border for the login_id_ text field.
  auto id_border = this->CreateRectangle(id_size, id_position);
  login_background_->addChild(id_border);

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
  const auto password_font_size = 28;
  const auto password_position = Size(300, 300);
  const auto password_size = Size(450, password_font_size * 1.75);
  login_password_ = TextField::create(
      "password", "fonts/GoogleSans-Regular.ttf", password_font_size);
  login_password_->setPosition(password_position);
  login_password_->setTouchAreaEnabled(true);
  login_password_->setTouchSize(password_size);
  login_password_->setPasswordEnabled(true);
  login_background_->addChild(login_password_);

  // Creates the border for the login_password_ text field.
  auto password_border =
      this->CreateRectangle(password_size, password_position);
  login_background_->addChild(password_border);

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
  auto login_button = Button::create(kLoginButtonImage, kSignUpButtonImage);
  login_button->setScale(.5);
  login_button->setPosition(Size(300, 200));
  login_background_->addChild(login_button);

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

  // Creates the back_button.
  auto back_button = Button::create(kJoinButtonImage, kSignUpButtonImage);
  back_button->setScale(.5);
  back_button->setPosition(Size(130, 500));
  login_background_->addChild(back_button);

  // Adds the event listener to return back to the kAuthMenuState.
  back_button->addTouchEventListener(
      [this](Ref* sender, Widget::TouchEventType type) {
        switch (type) {
          case Widget::TouchEventType::ENDED:
            state_ = kAuthMenuState;
            break;
          default:
            break;
        }
      });
}

// 1. Creates the background node.
// 2. Adds the layer title label: authentication.
// 3. Adds the id and password text fields and their event listeners.
// 4. Adds the back and login button.
void MainMenuScene::InitializeAuthenticationLayer() {
  // Creates a background to add on all of the cocos2d components. The
  // visiblity of this node should match kAuthMenuState and only active this
  // layers event listeners.

  // Creates and places the auth_background_.
  const auto auth_background_size = Size(500, 550);
  const auto auth_background_origin = Size(300, 300);
  auth_background_ =
      this->CreateRectangle(auth_background_size, auth_background_origin,
                            /*background_color=*/Color4F::BLACK);
  this->addChild(auth_background_, /*layer_index=*/10);

  // Creates the layer title label: authentication.
  auto layer_title = Label::createWithTTF("authentication",
                                          "fonts/GoogleSans-Regular.ttf", 48);
  layer_title->setPosition(Vec2(300, 550));
  auth_background_->addChild(layer_title);

  // Creates three buttons for the menu items (login,sign up, and anonymous sign
  // in).
  // For each menu item button, creates a normal and selected version and attach
  // the touch listener.

  // Creates the sign up menu item.
  const auto sign_up_normal_item = Sprite::create(kSignUpButtonImage);
  sign_up_normal_item->setContentSize(Size(450, 175));
  auto sign_up_selected = Sprite::create(kSignUpButtonImage);
  sign_up_normal_item->setContentSize(Size(450, 175));
  sign_up_selected->setColor(Color3B::GRAY);

  auto sign_up_item = MenuItemSprite::create(
      sign_up_normal_item, sign_up_selected, [this](Ref* sender) {
        auto node = dynamic_cast<Node*>(sender);
        if (node != nullptr) {
          state_ = kSignUpState;
        }
      });
  sign_up_item->setTag(0);

  // Creates the login menu item.
  const auto login_normal_item = Sprite::create(kLoginButtonImage);
  login_normal_item->setContentSize(Size(450, 175));
  auto login_selected_item = Sprite::create(kLoginButtonImage);
  login_normal_item->setContentSize(Size(450, 175));
  login_selected_item->setColor(Color3B::GRAY);

  auto login_item = MenuItemSprite::create(
      login_normal_item, login_selected_item, [this](Ref* sender) {
        auto node = dynamic_cast<Node*>(sender);
        if (node != nullptr) {
          state_ = kLoginState;
        }
      });
  login_item->setTag(1);

  // Creates the skip login menu item.
  const auto skip_login_normal_item = Sprite::create(kJoinButtonImage);
  skip_login_normal_item->setContentSize(Size(450, 175));
  auto skip_login_selected_item = Sprite::create(kJoinButtonImage);
  skip_login_selected_item->setContentSize(Size(450, 175));
  skip_login_selected_item->setColor(Color3B::GRAY);

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
  menu->setPosition(Size(290, 260));
  menu->setContentSize(Size(100, 200));
  menu->setScale(.8, .7);
  menu->alignItemsVerticallyWithPadding(30.0f);
  auth_background_->addChild(menu);
}

// Updates the user record variables to reflect what is in the database.
void MainMenuScene::UpdateUserRecord() {
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
  user_record_label_->setString("W:" + to_string(user_wins_) +
                                " L:" + to_string(user_loses_) +
                                " T:" + to_string(user_ties_));
}

// Creates a rectangle of Size size and centered on the origin.
// Optional parameters: background_color, border_color, border_thickness.
DrawNode* MainMenuScene::CreateRectangle(Size size, Vec2 origin,
                                         Color4F background_color,
                                         Color4F border_color,
                                         int border_thickness) {
  // Creates the corners of the rectangle.
  Vec2 corners[4];
  corners[0] = Vec2(origin.x - (size.width / 2), origin.y - (size.height / 2));
  corners[1] = Vec2(origin.x - (size.width / 2), origin.y + (size.height / 2));
  corners[2] = Vec2(origin.x + (size.width / 2), origin.y + (size.height / 2));
  corners[3] = Vec2(origin.x + (size.width / 2), origin.y - (size.height / 2));

  // Draws the rectangle.
  DrawNode* rectangle = DrawNode::create();
  rectangle->drawPolygon(corners, /*number_of_points=*/4, background_color,
                         border_thickness, border_color);
  return rectangle;
}

// Initialize the user records in the database.
void MainMenuScene::InitializeUserRecord() {
  ref_ = database_->GetReference("users").Child(user_uid_);
  auto future_wins = ref_.Child("wins").SetValue(0);
  auto future_loses = ref_.Child("loses").SetValue(0);
  auto future_ties = ref_.Child("ties").SetValue(0);
  WaitForCompletion(future_wins, "setUserWinsData");
  WaitForCompletion(future_loses, "setUserLosesData");
  WaitForCompletion(future_ties, "setUserTiesData");
  user_wins_ = 0;
  user_loses_ = 0;
  user_ties_ = 0;
  user_record_label_->setString("W:" + to_string(user_wins_) +
                                " L:" + to_string(user_loses_) +
                                " T:" + to_string(user_ties_));
}

// Overriding the onEnter method to update the user_record on reenter.
void MainMenuScene::onEnter() {
  // If the scene is entering from the game, UpdateUserRecords() and change
  // state_ back to kGameMenuState.
  if (state_ == kRunGameState) {
    this->UpdateUserRecord();
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
// Returns kAuthMenuState. This will be the default update method and
// immediately swap to auth state. TODO(grantpostma): have this display a
// loading screen before swapping.
MainMenuScene::kSceneState MainMenuScene::UpdateInitialize() {
  return kAuthMenuState;
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
      this->UpdateUserRecord();

      return kGameMenuState;
    } else {
      // Changes login_error_label_ to display the user_result_ future errored.
      login_error_label_->setString("invalid credentials");
      return kLoginState;
    }
  } else {
    return kLoginState;
  }
}

// Updates the layer and stays in this state until user_result_ completes.
// Initializes the user if the user_result_ is valid. Updates the error message
// and returns back to kSignUpState if the future user_result_ errored.
MainMenuScene::kSceneState MainMenuScene::UpdateSignUp() {
  this->UpdateLayer(state_);
  if (user_result_.status() == firebase::kFutureStatusComplete) {
    if (user_result_.error() == firebase::auth::kAuthErrorNone) {
      // Initializes user variables and stores them in the database.
      user_ = *user_result_.result();
      user_uid_ = GenerateUid(10);
      this->InitializeUserRecord();

      return kGameMenuState;
    } else {
      // Changes sign_up_error_label_ to display the user_result_ future
      // errored.
      sign_up_error_label_->setString("sign up failed");
      return kSignUpState;
    }
  } else {
    return kSignUpState;
  }
}
// Updates the layer and stays in this state until user_result_ completes.
// Initializes the user if the user_result_ is valid. Otherwise, return back to
// kAuthMenuState.
MainMenuScene::kSceneState MainMenuScene::UpdateSkipLogin() {
  if (user_result_.status() == firebase::kFutureStatusComplete) {
    if (user_result_.error() == firebase::auth::kAuthErrorNone) {
      // Initializes user variables and stores them in the database.
      user_ = *user_result_.result();
      user_uid_ = GenerateUid(10);
      this->InitializeUserRecord();

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
// Updates the auth_,login_ and sign_up_ layer based on state.
void MainMenuScene::UpdateLayer(MainMenuScene::kSceneState state) {
  auth_background_->setVisible(state == kAuthMenuState);
  login_background_->setVisible(state == kLoginState);
  sign_up_background_->setVisible(state == kSignUpState);
}