#ifndef TICTACTOE_DEMO_CLASSES_TICTACTOE_SCENE_H_
#define TICTACTOE_DEMO_CLASSES_TICTACTOE_SCENE_H_

#include "cocos2d.h"

class TicTacToe : public cocos2d::Layer {
 public:
  // Builds a simple scene that uses the bottom left cordinate point as (0,0)
  // and can have sprites, labels and nodes added onto it.
  static cocos2d::Scene* createScene(char*);
  // Initializes the instance of a Node and returns a boolean based on if it was
  // successful in doing so.
  bool init() override;
  // Called automatically every frame. The update is scheduled in `init()`.
  void update(float) override;
  // Defines a create type for a specific type, in this case a Layer.
  CREATE_FUNC(TicTacToe);
};
#endif  // TICTACTOE_DEMO_CLASSES_TICTACTOE_SCENE_H_