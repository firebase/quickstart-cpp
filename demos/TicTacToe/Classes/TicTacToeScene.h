#include "cocos2d.h"

class TicTacToe : public cocos2d::Layer {
 public:
  // Builds a simple scene that uses the bottom left cordinate point as (0,0)
  // and can have sprites, labels and nodes added onto it.
  static cocos2d::Scene* createScene();
  // Initializes the instance of a Node and returns a boolean based on if it was
  // successful in doing so.
  bool init() override;
  // Defines a create type for a specific type, in this case a Layer.
  CREATE_FUNC(TicTacToe);
};