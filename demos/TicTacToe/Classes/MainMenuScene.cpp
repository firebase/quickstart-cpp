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

#include "MainMenuScene.h"

#include <regex>

#include "TicTacToeScene.h"

static const char* kCreateGameImage = "create_game.png";
static const char* kTextFieldBorderImage = "text_field_border.png";
static const char* kJoinButtonImage = "join_game.png";
static const char* kLoginButtonImage = "login.png";
static const char* kLogoutButtonImage = "logout.png";
static const char* kSignUpButtonImage = "sign_up.png";

// Regex that will validate if the email entered is a valid email pattern.
const std::regex email_pattern("(\\w+)(\\.|_)?(\\w*)@(\\w+)(\\.(\\w+))+");
USING_NS_CC;

void LogMessage(const char* format, ...) {
  va_list list;
  va_start(list, format);
  vprintf(format, list);
  va_end(list);
  printf("\n");
  fflush(stdout);
}

void ProcessEvents(int msec) {
#ifdef _WIN32
  Sleep(msec);
#else
  usleep(msec * 1000);
#endif  // _WIN32
}

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

// Returns a random uid of a specified length.
std::string GenerateUid(std::size_t length) {
  const std::string kCharacters = "0123456789abcdefghjkmnpqrstuvwxyz";

  std::random_device random_device;
  std::mt19937 generator(random_device());
  std::uniform_int_distribution<> distribution(0, kCharacters.size() - 1);

  std::string generate_uid;

  for (std::size_t i = 0; i < length; ++i) {
    generate_uid += kCharacters[distribution(generator)];
  }

  return generate_uid;
}

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

  // Call the function to initialize the firebase features.
  this->InitializeFirebase();

  // Create the background to add all of the authentication elements on. The
  // visiblity of this node should match kAuthState, disable any
  // touch_listeners when not in this state.
  auth_background_ = DrawNode::create();
  auto auth_background_border = DrawNode::create();

  const auto auth_background_size = Size(500, 550);
  const auto auth_background_origin = Size(50, 50);
  Vec2 auth_background_corners[4];
  auth_background_corners[0] =
      Vec2(auth_background_origin.width, auth_background_origin.height);
  auth_background_corners[1] =
      Vec2(auth_background_origin.width + auth_background_size.width,
           auth_background_origin.height);
  auth_background_corners[2] =
      Vec2(auth_background_origin.width + auth_background_size.width,
           auth_background_origin.height + auth_background_size.height);
  auth_background_corners[3] =
      Vec2(auth_background_origin.width,
           auth_background_origin.height + auth_background_size.height);

  Color4F white(1, 1, 1, 1);
  auth_background_->drawPolygon(auth_background_corners, 4, Color4F::BLACK, 1,
                                Color4F::BLACK);
  auth_background_border->drawPolygon(auth_background_corners, 4,
                                      Color4F(0, 0, 0, 0), 1, Color4F::WHITE);
  auth_background_->addChild(auth_background_border);

  this->addChild(auth_background_, /*layer_index=*/10);

  // Label the background as Authentication.
  auto auth_label = Label::createWithSystemFont("authentication", "Arial", 48);
  auth_label->setPosition(Vec2(300, 550));
  auth_background_->addChild(auth_label);

  // Label to print out all of the login errors.
  invalid_login_label_ = Label::createWithSystemFont("", "Arial", 24);
  invalid_login_label_->setTextColor(Color4B::RED);
  invalid_login_label_->setPosition(Vec2(300, 220));
  auth_background_->addChild(invalid_login_label_);

  // Label to display the users record.
  user_record_label_ = Label::createWithSystemFont("", "Arial", 24);
  user_record_label_->setAlignment(TextHAlignment::RIGHT);
  user_record_label_->setTextColor(Color4B::WHITE);
  user_record_label_->setPosition(Vec2(500, 600));
  this->addChild(user_record_label_);

  // Label for anonymous sign in.
  auto anonymous_login_label =
      Label::createWithSystemFont("anonymous sign-in", "Arial", 18);
  anonymous_login_label->setTextColor(Color4B::WHITE);
  anonymous_login_label->setPosition(Vec2(460, 75));

  auto anonymous_label_touch_listener = EventListenerTouchOneByOne::create();

  anonymous_label_touch_listener->onTouchBegan =
      [this](cocos2d::Touch* touch, cocos2d::Event* event) -> bool {
    // Returns false, not consuming the event, to exit the layer if
    // current_state_ is not in the kAuthState or is switching states.
    if (previous_state_ != current_state_ || current_state_ != kAuthState) {
      return false;
    }
    const auto bounds = event->getCurrentTarget()->getBoundingBox();
    const auto point = touch->getLocation();
    if (bounds.containsPoint(point)) {
      // Use anonymous sign in for the user.
      user_result_ = auth_->SignInAnonymously();
      current_state_ = kWaitingAnonymousState;
    }

    return true;
  };

  // Attach the touch listener to the text field.
  Director::getInstance()
      ->getEventDispatcher()
      ->addEventListenerWithSceneGraphPriority(anonymous_label_touch_listener,
                                               anonymous_login_label);
  auth_background_->addChild(anonymous_login_label);

  // Extract the origin, size and position of the text field so that the
  // border can be created based on those values.
  const auto email_text_field_origin = Vec2(0, 0);
  const auto email_text_field_position = Size(110, 350);
  const auto text_field_padding = 20;
  const auto email_font_size = 36.0;
  const auto email_text_field_size = Size(400, 2 * email_font_size);

  // Set up the constraints of the border so it surrounds the text box.
  Vec2 email_border_corners[4] = {
      Vec2(email_text_field_position.width - text_field_padding,
           email_text_field_position.height),
      Vec2(email_text_field_position.width + email_text_field_size.width,
           email_text_field_position.height),
      Vec2(email_text_field_position.width + email_text_field_size.width,
           email_text_field_position.height + email_text_field_size.height),
      Vec2(email_text_field_position.width - text_field_padding,
           email_text_field_position.height + email_text_field_size.height)};

  // Create a text field border and add it around the text field.
  auto email_text_field_border = DrawNode::create();
  email_text_field_border->drawPolygon(email_border_corners, 4,
                                       Color4F(0, 0, 0, 0), 1, Color4F::WHITE);

  // Create a text field to enter the user's email.
  email_text_field_ = cocos2d::TextFieldTTF::textFieldWithPlaceHolder(
      "enter an email address", email_text_field_size, TextHAlignment::LEFT,
      "Arial", email_font_size);
  email_text_field_->setPosition(email_text_field_position);
  email_text_field_->setAnchorPoint(Vec2(0, 0));
  email_text_field_->setColorSpaceHolder(Color3B::GRAY);
  email_text_field_->setDelegate(this);

  auto email_text_field_touch_listener = EventListenerTouchOneByOne::create();

  email_text_field_touch_listener->onTouchBegan =
      [this](cocos2d::Touch* touch, cocos2d::Event* event) -> bool {
    // Returns if the layer is not in the auth state or is switching states.
    if (previous_state_ != current_state_ || current_state_ != kAuthState) {
      return true;
      // Returns false, not consuming the event, to exit the layer if
      // current_state_ is not in the kAuthState or is switching states.
      if (previous_state_ != current_state_ || current_state_ != kAuthState) {
        return false;
      }
      const auto bounds = event->getCurrentTarget()->getBoundingBox();
      const auto point = touch->getLocation();
      if (bounds.containsPoint(point)) {
        // Show the on screen keyboard and places character inputs into the text
        // field.
        auto text_field =
            dynamic_cast<TextFieldTTF*>(event->getCurrentTarget());
        text_field->setCursorEnabled(true);
        text_field->attachWithIME();
      } else {
        auto text_field =
            dynamic_cast<TextFieldTTF*>(event->getCurrentTarget());
        text_field->setCursorEnabled(false);
        text_field->detachWithIME();
      }

      return true;
    };

    // Attach the touch listener to the text field.
    Director::getInstance()
        ->getEventDispatcher()
        ->addEventListenerWithSceneGraphPriority(
            email_text_field_touch_listener, email_text_field_);

    auth_background_->addChild(email_text_field_, /*layer_index=*/1);
    auth_background_->addChild(email_text_field_border, /*layer_index=*/1);

    // Extract the origin, size and position of the text field so that the
    // border can be created based on those values.
    const auto password_text_field_origin = Vec2(0, 0);
    const auto password_text_field_position = Size(110, 250);
    const auto password_font_size = 36.0;
    const auto password_text_field_size = Size(400, 2 * password_font_size);

    // Set up the constraints of the border so it surrounds the text box.
    Vec2 password_border_corners[4] = {
        Vec2(password_text_field_position.width - text_field_padding,
             password_text_field_position.height),
        Vec2(
            password_text_field_position.width + password_text_field_size.width,
            password_text_field_position.height),
        Vec2(
            password_text_field_position.width + password_text_field_size.width,
            password_text_field_position.height +
                password_text_field_size.height),
        Vec2(password_text_field_position.width - text_field_padding,
             password_text_field_position.height +
                 password_text_field_size.height)};

    // Create a text field border and add it around the text field.
    auto password_text_field_border = DrawNode::create();
    password_text_field_border->drawPolygon(
        password_border_corners, 4, Color4F(0, 0, 0, 0), 1, Color4F::WHITE);

    // Create a text field to enter the user's password.
    password_text_field_ = cocos2d::TextFieldTTF::textFieldWithPlaceHolder(
        "enter a password", password_text_field_size, TextHAlignment::LEFT,
        "Arial", password_font_size);
    password_text_field_->setPosition(password_text_field_position);
    password_text_field_->setAnchorPoint(Vec2(0, 0));
    password_text_field_->setColorSpaceHolder(Color3B::GRAY);
    password_text_field_->setSecureTextEntry(true);
    password_text_field_->setDelegate(this);

    auto password_text_field_touch_listener =
        EventListenerTouchOneByOne::create();

    password_text_field_touch_listener->onTouchBegan =
        [this](cocos2d::Touch* touch, cocos2d::Event* event) -> bool {
      // Returns false, not consuming the event, to exit the layer if
      // current_state_ is not in the kAuthState or is switching states.
      if (previous_state_ != current_state_ || current_state_ != kAuthState) {
        return false;
      }
      const auto bounds = event->getCurrentTarget()->getBoundingBox();
      const auto point = touch->getLocation();
      if (bounds.containsPoint(point)) {
        // Shows the on screen keyboard and places character inputs into the
        // text field.
        auto text_field =
            dynamic_cast<TextFieldTTF*>(event->getCurrentTarget());
        text_field->setCursorEnabled(true);
        text_field->attachWithIME();
      } else {
        auto text_field =
            dynamic_cast<TextFieldTTF*>(event->getCurrentTarget());
        text_field->setCursorEnabled(false);
        text_field->detachWithIME();
      }

      return true;
    };

    auth_background_->addChild(password_text_field_, /*layer_index=*/1);
    auth_background_->addChild(password_text_field_border, /*layer_index=*/1);

    // Attach the touch listener to the text field.
    Director::getInstance()
        ->getEventDispatcher()
        ->addEventListenerWithSceneGraphPriority(
            password_text_field_touch_listener, password_text_field_);

    // Create the login button and give it a position, anchor point and
    // touch_listener.
    auto login_button = Sprite::create(kLoginButtonImage);
    login_button->setPosition(90, 120);
    login_button->setAnchorPoint(Vec2(0, 0));
    login_button->setContentSize(Size(200, 75));

    // Create a button listener to handle the touch event.
    auto login_button_touch_listener = EventListenerTouchOneByOne::create();

    // Transition from kAuthState to kWaitingLoginState on button press and set
    // user_result_ to SignInWithEmailAndPassword future result.
    login_button_touch_listener->onTouchBegan = [this](Touch* touch,
                                                       Event* event) -> bool {
      // Returns false, not consuming the event, to exit the layer if
      // current_state_ is not in the kAuthState or is switching states.
      if (previous_state_ != current_state_ || current_state_ != kAuthState) {
        return false;
      }
      const auto bounds = event->getCurrentTarget()->getBoundingBox();
      const auto point = touch->getLocation();
      if (bounds.containsPoint(point)) {
        // Check the string for a valid email pattern.
        if (!std::regex_match(email_text_field_->getString(), email_pattern)) {
          invalid_login_label_->setString("invalid email address");
        } else if (password_text_field_->getString().length() < 8) {
          invalid_login_label_->setString(
              "password must be at least 8 characters long");
        } else {
          user_result_ = auth_->SignInWithEmailAndPassword(
              email_text_field_->getString().c_str(),
              password_text_field_->getString().c_str());
          current_state_ = kWaitingLoginState;
        }
      }
      return true;
    };

    // Attach the touch listener to the login button.
    Director::getInstance()
        ->getEventDispatcher()
        ->addEventListenerWithSceneGraphPriority(login_button_touch_listener,
                                                 login_button);
    auth_background_->addChild(login_button, /*layer_index=*/1);

    // Create the sign_up button and give it a position, anchor point and
    // touch_listener.
    auto sign_up_button = Sprite::create(kSignUpButtonImage);
    sign_up_button->setPosition(310, 120);
    sign_up_button->setAnchorPoint(Vec2(0, 0));
    sign_up_button->setContentSize(Size(200, 75));

    // Create a button listener to handle the touch event.
    auto sign_up_button_touch_listener = EventListenerTouchOneByOne::create();

    // Transition from kAuthState to kWaitingSignUpState on button press and set
    // user_result_ to CreateUserWithEmailAndPassword future result.
    sign_up_button_touch_listener->onTouchBegan = [this](Touch* touch,
                                                         Event* event) -> bool {
      // Returns false, not consuming the event, to exit the layer if
      // current_state_ is not in the kAuthState or is switching states.
      if (previous_state_ != current_state_ || current_state_ != kAuthState) {
        return false;
      }
      const auto bounds = event->getCurrentTarget()->getBoundingBox();
      const auto point = touch->getLocation();
      if (bounds.containsPoint(point)) {
        // Check the string for a valid email pattern.
        if (!std::regex_match(email_text_field_->getString(), email_pattern)) {
          invalid_login_label_->setString("invalid email address");
        } else if (password_text_field_->getString().length() < 8) {
          invalid_login_label_->setString(
              "password must be at least 8 characters long");
        } else {
          user_result_ = auth_->CreateUserWithEmailAndPassword(
              email_text_field_->getString().c_str(),
              password_text_field_->getString().c_str());
          current_state_ = kWaitingSignUpState;
        }
      }
      return true;
    };

    // Attach the touch listener to the sign_up button.
    Director::getInstance()
        ->getEventDispatcher()
        ->addEventListenerWithSceneGraphPriority(sign_up_button_touch_listener,
                                                 sign_up_button);
    auth_background_->addChild(sign_up_button, /*layer_index=*/1);

    // Create, set the position and assign a placeholder to the text
    // field for the user to enter the join game uuid.
    TextFieldTTF* join_text_field =
        cocos2d::TextFieldTTF::textFieldWithPlaceHolder(
            "code", cocos2d::Size(200, 100), TextHAlignment::LEFT, "Arial",
            55.0);
    join_text_field->setPosition(420, 45);
    join_text_field->setAnchorPoint(Vec2(0, 0));
    join_text_field->setColorSpaceHolder(Color3B::WHITE);
    join_text_field->setDelegate(this);

    auto join_text_field_border = Sprite::create(kTextFieldBorderImage);
    join_text_field_border->setPosition(390, 50);
    join_text_field_border->setAnchorPoint(Vec2(0, 0));
    join_text_field_border->setScale(.53f);
    this->addChild(join_text_field_border, /*layer_index=*/0);

    // Create a touch listener to handle the touch event.
    auto join_text_field_touch_listener = EventListenerTouchOneByOne::create();

    join_text_field_touch_listener->onTouchBegan =
        [join_text_field, this](cocos2d::Touch* touch,
                                cocos2d::Event* event) -> bool {
      // Returns false, not consuming the event, to exit the layer if
      // current_state_ is not in the kGameMenuState or is switching states.
      if (previous_state_ != current_state_ ||
          current_state_ != kGameMenuState) {
        return false;
      }
      const auto bounds = event->getCurrentTarget()->getBoundingBox();
      const auto point = touch->getLocation();
      if (bounds.containsPoint(point)) {
        // Show the on screen keyboard and places character inputs into the text
        // field.
        auto str = join_text_field->getString();
        auto text_field =
            dynamic_cast<TextFieldTTF*>(event->getCurrentTarget());
        text_field->setCursorEnabled(true);
        text_field->attachWithIME();
      } else {
        auto text_field =
            dynamic_cast<TextFieldTTF*>(event->getCurrentTarget());
        text_field->setCursorEnabled(false);
        text_field->detachWithIME();
      }

      return true;
    };

    // Attach the touch listener to the text field.
    Director::getInstance()
        ->getEventDispatcher()
        ->addEventListenerWithSceneGraphPriority(join_text_field_touch_listener,
                                                 join_text_field);

    // Creates a sprite for the create button and sets its position to the
    // center of the screen. TODO(grantpostma): Dynamically choose the location.
    auto create_button = Sprite::create(kCreateGameImage);
    create_button->setPosition(25, 200);
    create_button->setAnchorPoint(Vec2(0, 0));

    // Create a button listener to handle the touch event.
    auto create_button_touch_listener = EventListenerTouchOneByOne::create();

    // Set the onTouchBegan event up to a lambda tha will replace the
    // MainMenu scene with a TicTacToe scene.
    create_button_touch_listener->onTouchBegan =
        [this, join_text_field](Touch* touch, Event* event) -> bool {
      // Returns false, not consuming the event, to exit the layer if
      // current_state_ is not in the kGameMenuState or is switching states.
      if (previous_state_ != current_state_ ||
          current_state_ != kGameMenuState) {
        return false;
      };
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

    // Create a button listener to handle the touch event.
    auto logout_button_touch_listener = EventListenerTouchOneByOne::create();

    // Set the onTouchBegan event up to a lambda tha will replace the
    // MainMenu scene with a TicTacToe scene.
    logout_button_touch_listener->onTouchBegan = [this](Touch* touch,
                                                        Event* event) -> bool {
      // Returns false, not consuming the event, to exit the layer if
      // current_state_ is not in the kGameMenuState or is switching states.
      if (previous_state_ != current_state_ ||
          current_state_ != kGameMenuState) {
        return false;
      }
      const auto bounds = event->getCurrentTarget()->getBoundingBox();
      const auto point = touch->getLocation();

      // Replaces the scene with a new TicTacToe scene if the touched point is
      // within the bounds of the button.
      if (bounds.containsPoint(point)) {
        current_state_ = kAuthState;
        user_uid_ = "";
        user_ = nullptr;
        invalid_login_label_->setString("");
        email_text_field_->setString("");
        password_text_field_->setString("");
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

    // Create a button listener to handle the touch event.
    auto join_button_touch_listener = EventListenerTouchOneByOne::create();

    // Set the onTouchBegan event up to a lambda tha will replace the
    // MainMenu scene with a TicTacToe scene and pass in join_text_field string.
    join_button_touch_listener->onTouchBegan =
        [join_text_field, this](Touch* touch, Event* event) -> bool {
      // Returns false, not consuming the event, to exit the layer if
      // current_state_ is not in the kGameMenuState or is switching states.
      if (previous_state_ != current_state_ ||
          current_state_ != kGameMenuState) {
        return false;
      }
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
  void
  MainMenuScene::InitializeFirebase() {
    LogMessage("Initialize Firebase App.");
    ::firebase::App* app;

#if defined(_ANDROID_)
    app = ::firebase::App::Create(GetJniEnv(), GetActivity());
#else
    app = ::firebase::App::Create();
#endif  // defined(ANDROID)

    LogMessage("Initialize Firebase Auth and Firebase Database.");

    // Use ModuleInitializer to initialize both Auth and Database, ensuring no
    // dependencies are missing.
    database_ = nullptr;
    auth_ = nullptr;
    void* initialize_targets[] = {&auth_, &database_};

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

    ::firebase::ModuleInitializer initializer;
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

  // Updates the previous_state_ when current_state_ != previous_state_:
  //
  // switch (current_state_)
  // (1) kAuthState: makes the auth_background_ visable.
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

            this->InitializeUserRecord();

            current_state_ = kGameMenuState;

          } else {
            // Change invalid_login_label_ to display the user_create failed.
            invalid_login_label_->setString("invalid sign up");
            current_state_ = kAuthState;
          }
        }
      } else if (current_state_ == kWaitingLoginState) {
        if (user_result_.status() == firebase::kFutureStatusComplete) {
          if (user_result_.error() == firebase::auth::kAuthErrorNone) {
            user_ = *user_result_.result();
            user_uid_ = user_->uid();

            this->UpdateUserRecord();

            current_state_ = kGameMenuState;
          } else {
            // Change invalid_login_label_ to display the auth_result errored.
            auto err = user_result_.error_message();
            invalid_login_label_->setString("invalid login");
            current_state_ = kAuthState;
          }
        }
      } else if (current_state_ == kAuthState) {
        // Sign out logic, adding auth screen.
        auth_background_->setVisible(true);
        user_ = nullptr;
        previous_state_ = current_state_;
      } else if (current_state_ == kGameMenuState) {
        // Removes the authentication screen.
        auth_background_->setVisible(false);
        previous_state_ = current_state_;
      }
    }
    return;
  }
