#include "MainMenuScene.h"

#include "TicTacToeScene.h"

USING_NS_CC;

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
  // Creates a sprite for the create button and sets its position to the center
  // of the screen. TODO(grantpostma): Dynamically choose the location.
  auto create_button = Sprite::create("create_game.png");
  create_button->setPosition(300, 350);
  // Create a button listener to handle the touch event.
  auto create_button_touch_listener = EventListenerTouchOneByOne::create();
  // Setting the onTouchBegan event up to a lambda tha will replace the MainMenu
  // scene with a TicTacToe scene.
  create_button_touch_listener->onTouchBegan = [](Touch* touch,
                                                  Event* event) -> bool {
    auto bounds = event->getCurrentTarget()->getBoundingBox();
    auto point = touch->getLocation();
    // Replaces the scene with a new TicTacToe scene if the touched point is
    // within the bounds of the button.
    if (bounds.containsPoint(point)) {
      Director::getInstance()->replaceScene(
          TicTacToe::createScene(std::string()));
    }

    return true;
  };
  // Attaching the touch listener to the create game button.
  Director::getInstance()
      ->getEventDispatcher()
      ->addEventListenerWithSceneGraphPriority(create_button_touch_listener,
                                               create_button);

  // Creating, setting the position and assigning a placeholder to the text
  // field for entering the join game uuid.
  TextFieldTTF* join_text_field =
      cocos2d::TextFieldTTF::textFieldWithPlaceHolder(
          "Join Game url", cocos2d::Size(400, 200), TextHAlignment::RIGHT,
          "Arial", 42.0);
  join_text_field->setPosition(100, 100);
  join_text_field->setColorSpaceHolder(Color3B::GRAY);
  join_text_field->setDelegate(this);

  // Create a touch listener to handle the touch event. TODO(grantpostma): add a
  // focus bar when selecting inside the text field's bounding box.
  auto text_field_touch_listener = EventListenerTouchOneByOne::create();

  text_field_touch_listener->onTouchBegan =
      [join_text_field](cocos2d::Touch* touch, cocos2d::Event* event) -> bool {
    auto bounds = event->getCurrentTarget()->getBoundingBox();
    auto point = touch->getLocation();
    if (bounds.containsPoint(point)) {
      // Show the on screen keyboard and places character inputs into the text
      // field.
      auto str = join_text_field->getString();
      auto textField = dynamic_cast<TextFieldTTF*>(event->getCurrentTarget());
      textField->attachWithIME();
    }

    return true;
  };

  // Attaching the touch listener to the text field.
  Director::getInstance()
      ->getEventDispatcher()
      ->addEventListenerWithSceneGraphPriority(text_field_touch_listener,
                                               join_text_field);

  // Creates a sprite for the join button and sets its position to the center
  // of the screen. TODO(grantpostma): Dynamically choose the location.
  auto join_button = Sprite::create("join_game.png");
  join_button->setPosition(450, 100);

  // Create a button listener to handle the touch event.
  auto join_button_touch_listener = EventListenerTouchOneByOne::create();
  // Setting the onTouchBegan event up to a lambda tha will replace the MainMenu
  // scene with a TicTacToe scene and pass in join_text_field string.
  join_button_touch_listener->onTouchBegan =
      [join_text_field](Touch* touch, Event* event) -> bool {
    auto bounds = event->getCurrentTarget()->getBoundingBox();
    auto point = touch->getLocation();
    if (bounds.containsPoint(point)) {
      // Getting and converting the join_text_field string to a char*.
      std::string join_text_field_string = join_text_field->getString();

      Director::getInstance()->replaceScene(
          TicTacToe::createScene(join_text_field_string));
    }
    return true;
  };
  // Attaching the touch listener to the join button.
  Director::getInstance()
      ->getEventDispatcher()
      ->addEventListenerWithSceneGraphPriority(join_button_touch_listener,
                                               join_button);
  // Attaching the create button, join button and join text field to the
  // MainMenu scene.
  this->addChild(create_button);
  this->addChild(join_button);
  this->addChild(join_text_field);

  return true;
}
