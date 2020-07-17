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

  // Label to display the users record.
  user_record_label_ = Label::createWithSystemFont("", "Arial", 24);
  user_record_label_->setAlignment(TextHAlignment::RIGHT);
  user_record_label_->setTextColor(Color4B::WHITE);
  user_record_label_->setPosition(Vec2(500, 600));
  this->addChild(user_record_label_);

  auto join_text_field_position = Size(480, 95);
  auto join_text_field_size = Size(180, 80);
  auto join_text_field = TextField::create("code", "Arial", 48);
  join_text_field->setTextHorizontalAlignment(TextHAlignment::CENTER);
  join_text_field->setPosition(join_text_field_position);
  join_text_field->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
  join_text_field->setTouchSize(join_text_field_size);
  join_text_field->setTouchAreaEnabled(true);
  join_text_field->setMaxLength(/*max_characters=*/4);
  join_text_field->setMaxLengthEnabled(true);

  // Adds the event listener to handle the actions for a textfield.
  join_text_field->addEventListener([this](Ref* sender,
                                           TextField::EventType type) {
    auto join_text_field = dynamic_cast<TextField*>(sender);
    string join_text_field_string = join_text_field->getString();
    // Transforms the letter casing to uppercase.
    std::transform(join_text_field_string.begin(), join_text_field_string.end(),
                   join_text_field_string.begin(), toupper);

    // Creates a repeating blink action for the cursor.
    switch (type) {
      case TextField::EventType::ATTACH_WITH_IME:
        // Runs the repeated blinking cursor action.
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

  // Set up the constraints of the border so it surrounds the text box.
  const auto pos = join_text_field_position;
  const auto size = join_text_field_size;
  const Vec2 join_text_border_corners[4] = {
      Vec2(pos.width - size.width / 2, pos.height - size.height / 2),
      Vec2(pos.width + size.width / 2, pos.height - size.height / 2),
      Vec2(pos.width + size.width / 2, pos.height + size.height / 2),
      Vec2(pos.width - size.width / 2, pos.height + size.height / 2)};

  // Creates a border and adds it around the text field.
  auto join_text_field_border = DrawNode::create();
  join_text_field_border->drawPolygon(join_text_border_corners, 4,
                                      Color4F(0, 0, 0, 0), 1, Color4F::WHITE);
  this->addChild(join_text_field_border);

  // Creates a sprite for the create button and sets its position to the
  // center of the screen. TODO(grantpostma): Dynamically choose the location.
  auto create_button = Sprite::create(kCreateGameImage);
  create_button->setPosition(25, 200);
  create_button->setAnchorPoint(Vec2(0, 0));

  // Creates a button listener to handle the touch event.
  auto create_button_touch_listener = EventListenerTouchOneByOne::create();

  // Set the onTouchBegan event up to a lambda tha will replace the
  // MainMenu scene with a TicTacToe scene.
  create_button_touch_listener->onTouchBegan =
      [this, join_text_field](Touch* touch, Event* event) -> bool {
    const auto bounds = event->getCurrentTarget()->getBoundingBox();
    const auto point = touch->getLocation();

    // Replaces the scene with a new TicTacToe scene if the touched point is
    // within the bounds of the button.
    if (bounds.containsPoint(point)) {
      Director::getInstance()->pushScene(
          TicTacToe::createScene(std::string(), database_, user_uid_));
      join_text_field->setString("");
      current_state_ = kWaitingGameOutcome;
    }

    return true;
  };

  // Attach the touch listener to the create game button.
  Director::getInstance()
      ->getEventDispatcher()
      ->addEventListenerWithSceneGraphPriority(create_button_touch_listener,
                                               create_button);

  // Creates a sprite for the logout button and sets its position to the
  auto logout_button = Sprite::create(kLogoutButtonImage);
  logout_button->setPosition(25, 575);
  logout_button->setAnchorPoint(Vec2(0, 0));
  logout_button->setContentSize(Size(125, 50));

  // Creates a button listener to handle the touch event.
  auto logout_button_touch_listener = EventListenerTouchOneByOne::create();

  // Set the onTouchBegan event up to a lambda tha will replace the
  // MainMenu scene with a TicTacToe scene.
  logout_button_touch_listener->onTouchBegan = [this](Touch* touch,
                                                      Event* event) -> bool {
    const auto bounds = event->getCurrentTarget()->getBoundingBox();
    const auto point = touch->getLocation();

    // Replaces the scene with a new TicTacToe scene if the touched point is
    // within the bounds of the button.
    if (bounds.containsPoint(point)) {
      current_state_ = kAuthMenuState;
      user_uid_ = "";
      user_ = nullptr;
      user_record_label_->setString("");
    }

    return true;
  };

  // Attach the touch listener to the logout game button.
  Director::getInstance()
      ->getEventDispatcher()
      ->addEventListenerWithSceneGraphPriority(logout_button_touch_listener,
                                               logout_button);

  // Creates a sprite for the join button and sets its position to the center
  // of the screen. TODO(grantpostma): Dynamically choose the location and set
  // size().
  auto join_button = Sprite::create(kJoinButtonImage);
  join_button->setPosition(25, 50);
  join_button->setAnchorPoint(Vec2(0, 0));
  join_button->setScale(1.3f);

  // Creates a button listener to handle the touch event.
  auto join_button_touch_listener = EventListenerTouchOneByOne::create();

  // Set the onTouchBegan event up to a lambda tha will replace the
  // MainMenu scene with a TicTacToe scene and pass in join_text_field string.
  join_button_touch_listener->onTouchBegan =
      [join_text_field, this](Touch* touch, Event* event) -> bool {
    const auto bounds = event->getCurrentTarget()->getBoundingBox();
    const auto point = touch->getLocation();
    if (bounds.containsPoint(point)) {
      // Get the string from join_text_field.
      std::string join_text_field_string = join_text_field->getString();
      if (join_text_field_string.length() == 4) {
        Director::getInstance()->pushScene(TicTacToe::createScene(
            join_text_field_string, database_, user_uid_));
        current_state_ = kWaitingGameOutcome;
        join_text_field->setString("");
      } else {
        join_text_field->setString("");
      }
    }
    return true;
  };

  // Attach the touch listener to the join button.
  Director::getInstance()
      ->getEventDispatcher()
      ->addEventListenerWithSceneGraphPriority(join_button_touch_listener,
                                               join_button);

  // Attach the create button, join button and join text field to the
  // MainMenu scene.
  this->addChild(create_button);
  this->addChild(join_button);
  this->addChild(logout_button);
  this->addChild(join_text_field, /*layer_index=*/1);

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
  auto layer_title = Label::createWithSystemFont("sign up", "Arial", 48);
  layer_title->setAnchorPoint(Vec2(.5, .5));
  layer_title->setPosition(Vec2(300, 475));
  sign_up_background_->addChild(layer_title);

  // Label to output sign up errors.
  sign_up_error_label_ = Label::createWithSystemFont("", "Arial", 24);
  sign_up_error_label_->setTextColor(Color4B::RED);
  sign_up_error_label_->setPosition(Vec2(300, 425));
  sign_up_background_->addChild(sign_up_error_label_);

  // Creates the sign_up_id_.
  const auto id_font_size = 28;
  const auto id_position = Size(300, 375);
  const auto id_size = Size(450, id_font_size * 1.75);
  sign_up_id_ = TextField::create("email", "Arial", id_font_size);
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
  sign_up_password_ =
      TextField::create("password", "Arial", password_font_size);
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
  sign_up_password_confirm_ = TextField::create("confirm password", "Arial",
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
            // user_result_ future and swaps to kWaitingSignUpState.
            if (!std::regex_match(sign_up_id_->getString(), email_pattern)) {
              sign_up_error_label_->setString("invalid email address");
            } else if (sign_up_password_->getString().length() < 8) {
              sign_up_error_label_->setString(
                  "password must be at least 8 characters long");
            } else if (sign_up_password_->getString() !=
                       sign_up_password_confirm_->getString()) {
              sign_up_error_label_->setString("passwords do not match");
            } else {
              sign_up_error_label_->setString("");
              user_result_ = auth_->CreateUserWithEmailAndPassword(
                  sign_up_id_->getString().c_str(),
                  sign_up_password_->getString().c_str());
              current_state_ = kWaitingSignUpState;
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

  // Adds the event listener to return back to the kAuthMenuState.
  back_button->addTouchEventListener(
      [this](Ref* sender, Widget::TouchEventType type) {
        switch (type) {
          case Widget::TouchEventType::ENDED:
            current_state_ = kAuthMenuState;
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
  auto layer_title = Label::createWithSystemFont("Login", "Arial", 48);
  layer_title->setAnchorPoint(Vec2(.5, .5));
  layer_title->setPosition(Vec2(300, 475));
  login_background_->addChild(layer_title);

  // Label to output login errors.
  login_error_label_ = Label::createWithSystemFont("", "Arial", 24);
  login_error_label_->setTextColor(Color4B::RED);
  login_error_label_->setPosition(Vec2(300, 425));
  login_background_->addChild(login_error_label_);

  // Creating the login_id_.
  const auto id_font_size = 28;
  const auto id_position = Size(300, 375);
  const auto id_size = Size(450, id_font_size * 1.75);
  login_id_ = TextField::create("email", "Arial", id_font_size);
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
  login_password_ = TextField::create("password", "Arial", password_font_size);
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
            // user_result_ future and swaps to kWaitingLoginState.
            if (!std::regex_match(login_id_->getString(), email_pattern)) {
              login_error_label_->setString("invalid email address");
            } else if (login_password_->getString().length() < 8) {
              login_error_label_->setString(
                  "password must be at least 8 characters long");
            } else {
              login_error_label_->setString("");
              user_result_ = auth_->SignInWithEmailAndPassword(
                  login_id_->getString().c_str(),
                  login_password_->getString().c_str());
              current_state_ = kWaitingLoginState;
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
            current_state_ = kAuthMenuState;
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
  auto layer_title = Label::createWithSystemFont("authentication", "Arial", 48);
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
          current_state_ = kSignUpState;
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
          current_state_ = kLoginState;
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
          current_state_ = kWaitingAnonymousState;
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
  // if the scene enter is from the game, updateUserRecords and change
  // current_state_.
  if (current_state_ == kWaitingGameOutcome) {
    this->UpdateUserRecord();
    current_state_ = kGameMenuState;
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

// Updates the previous_state_ when current_state_ != previous_state_:
//
// switch (current_state_)
// (1) kAuthMenuState: makes the auth_background_ visable.
// (2) kGameMenuState: makes the auth_background_ invisable.
// (3) kWaitingAnonymousState: waits for anonymous sign in then swaps to (1).
// (4) kWaitingSignUpState: waits for sign up future completion,
//     updates user variables, and swaps to (2).
// (5) kWaitingLoginState: waits for login future completion,
//     updates user variables, and swaps to (2).
// (6) kWaitingGameOutcome: waits for director to pop the TicTacToeScene.
void MainMenuScene::update(float /*delta*/) {
  if (current_state_ != previous_state_) {
    if (current_state_ == kWaitingAnonymousState) {
      if (user_result_.status() == firebase::kFutureStatusComplete) {
        if (user_result_.error() == firebase::auth::kAuthErrorNone) {
          user_ = *user_result_.result();
          user_uid_ = GenerateUid(10);

          this->InitializeUserRecord();

          current_state_ = kGameMenuState;
        }
      }
    } else if (current_state_ == kWaitingSignUpState) {
      if (user_result_.status() == firebase::kFutureStatusComplete) {
        if (user_result_.error() == firebase::auth::kAuthErrorNone) {
          user_ = *user_result_.result();
          user_uid_ = user_->uid();

          this->ClearAuthFields();
          this->InitializeUserRecord();

          current_state_ = kGameMenuState;

        } else {
          // Change invalid_login_label_ to display the user_create failed.
          sign_up_error_label_->setString("invalid credentials");
          current_state_ = kSignUpState;
        }
      }
    } else if (current_state_ == kWaitingLoginState) {
      if (user_result_.status() == firebase::kFutureStatusComplete) {
        if (user_result_.error() == firebase::auth::kAuthErrorNone) {
          user_ = *user_result_.result();
          user_uid_ = user_->uid();
          this->ClearAuthFields();
          this->UpdateUserRecord();

          current_state_ = kGameMenuState;
        } else {
          // Change invalid_login_label_ to display the auth_result errored.
          login_error_label_->setString("invalid credentials");
          current_state_ = kLoginState;
        }
      }
    } else if (current_state_ == kAuthMenuState) {
      // Sets the authentication layer visable and hides the login &
      // sign up layers.
      auth_background_->setVisible(true);
      login_background_->setVisible(false);
      sign_up_background_->setVisible(false);
      // Pauses all event touch listeners & then resumes the ones attached to
      // auth_background_.
      const auto event_dispatcher =
          Director::getInstance()->getEventDispatcher();
      event_dispatcher->pauseEventListenersForTarget(this,
                                                     /*recursive=*/true);
      event_dispatcher->resumeEventListenersForTarget(auth_background_,
                                                      /*recursive=*/true);
      user_ = nullptr;
      previous_state_ = current_state_;
    } else if (current_state_ == kLoginState) {
      // Sets the login layer visable and hides the authentication &
      // sign up layers.
      auth_background_->setVisible(false);
      sign_up_background_->setVisible(false);
      login_background_->setVisible(true);
      // Pauses all event touch listeners & then resumes the ones attached to
      // login_background_.
      const auto event_dispatcher =
          Director::getInstance()->getEventDispatcher();
      event_dispatcher->pauseEventListenersForTarget(this,
                                                     /*recursive=*/true);
      event_dispatcher->resumeEventListenersForTarget(login_background_,
                                                      /*recursive=*/true);
      user_ = nullptr;
      previous_state_ = current_state_;
    } else if (current_state_ == kSignUpState) {
      // Sets the sign up layer visable and hides the authentication &
      // login layers.
      auth_background_->setVisible(false);
      login_background_->setVisible(false);
      sign_up_background_->setVisible(true);

      // Pauses all event touch listeners & then resumes the ones attached to
      // sign_up_background_.
      const auto event_dispatcher =
          Director::getInstance()->getEventDispatcher();
      event_dispatcher->pauseEventListenersForTarget(this,
                                                     /*recursive=*/true);
      event_dispatcher->resumeEventListenersForTarget(sign_up_background_,
                                                      /*recursive=*/true);
      user_ = nullptr;
      previous_state_ = current_state_;
    } else if (current_state_ == kGameMenuState) {
      // hides the authentication,login, and sign up layers.
      auth_background_->setVisible(false);
      login_background_->setVisible(false);
      sign_up_background_->setVisible(false);
      const auto event_dispatcher =
          Director::getInstance()->getEventDispatcher();
      // Resumes all event touch listeners & then pauses the ones
      // attached to auth_background_.
      event_dispatcher->resumeEventListenersForTarget(this,
                                                      /*recursive=*/true);
      event_dispatcher->pauseEventListenersForTarget(auth_background_,
                                                     /*recursive=*/true);
      previous_state_ = current_state_;
    }
  }
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
