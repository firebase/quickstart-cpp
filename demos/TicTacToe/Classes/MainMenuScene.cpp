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
  create_button->setPosition(25, 200);
  create_button->setAnchorPoint(Vec2(0, 0));
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
          "code", cocos2d::Size(200, 100), TextHAlignment::LEFT, "Arial", 55.0);
  join_text_field->setPosition(420, 45);
  join_text_field->setAnchorPoint(Vec2(0, 0));
  join_text_field->setColorSpaceHolder(Color3B::WHITE);
  join_text_field->setDelegate(this);

  auto text_field_border = Sprite::create("text_field_border.png");
  text_field_border->setPosition(390, 50);
  text_field_border->setAnchorPoint(Vec2(0, 0));
  text_field_border->setScale(.53f);
  this->addChild(text_field_border, 0);
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
      auto text_field = dynamic_cast<TextFieldTTF*>(event->getCurrentTarget());
      text_field->setCursorEnabled(true);
      text_field->attachWithIME();
    } else {
      auto text_field = dynamic_cast<TextFieldTTF*>(event->getCurrentTarget());
      text_field->setCursorEnabled(false);
      text_field->detachWithIME();
    }

    return true;
  };

  // Attaching the touch listener to the text field.
  Director::getInstance()
      ->getEventDispatcher()
      ->addEventListenerWithSceneGraphPriority(text_field_touch_listener,
                                               join_text_field);

  // Creates a sprite for the join button and sets its position to the center
  // of the screen. TODO(grantpostma): Dynamically choose the location and set
  // size().
  auto join_button = Sprite::create("join_game.png");
  join_button->setPosition(25, 50);
  join_button->setAnchorPoint(Vec2(0, 0));
  join_button->setScale(1.3f);
  // Create a button listener to handle the touch event.
  auto join_button_touch_listener = EventListenerTouchOneByOne::create();
  // Setting the onTouchBegan event up to a lambda tha will replace the MainMenu
  // scene with a TicTacToe scene and pass in join_text_field string.
  join_button_touch_listener->onTouchBegan =
      [join_text_field](Touch* touch, Event* event) -> bool {
    auto bounds = event->getCurrentTarget()->getBoundingBox();
    auto point = touch->getLocation();
    if (bounds.containsPoint(point)) {
      // Getting the string from join_text_field.
      std::string join_text_field_string = join_text_field->getString();
      if (join_text_field_string.length() == 4) {
        Director::getInstance()->replaceScene(
            TicTacToe::createScene(join_text_field_string));
      } else {
        join_text_field->setString("");
      }
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
  this->addChild(join_text_field, 1);

  return true;
}
