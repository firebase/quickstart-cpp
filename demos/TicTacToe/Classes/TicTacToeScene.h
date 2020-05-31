#pragma once

#include "cocos2d.h"
#include <array>
#include <iostream>
#include <cstdlib>



class TicTacToe : public cocos2d::Layer
{
public:
    static cocos2d::Scene* createScene();
    virtual bool init();
    CREATE_FUNC(TicTacToe);


};