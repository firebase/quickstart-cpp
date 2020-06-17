#ifndef TICTACTOE_DEMO_CLASSES_MAINMENU_SCENE_H_
#define TICTACTOE_DEMO_CLASSES_MAINMENU_SCENE_H_

#include "cocos2d.h"
#include "ui/CocosGUI.h"

class MainMenuScene : public cocos2d::Layer, public cocos2d::TextFieldDelegate {
 public:
  // Builds a simple scene that uses the bottom left cordinate point as (0,0)
  // and can have sprites, labels and nodes added onto it.
  static cocos2d::Scene* createScene();
  // Initializes the instance of a Node and returns a boolean based on if it was
  // successful in doing so.
  virtual bool init();
  // Defines a create type for a specific type, in this case a Layer.
  CREATE_FUNC(MainMenuScene);
};

#endif  // TICTACTOE_DEMO_CLASSES_MAINMENU_SCENE_H_
