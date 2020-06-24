#ifndef TICTACTOE_DEMO_CLASSES_MAINMENU_SCENE_H_
#define TICTACTOE_DEMO_CLASSES_MAINMENU_SCENE_H_

#include "cocos2d.h"
#include "firebase/auth.h"
#include "firebase/database.h"
#include "firebase/future.h"
#include "firebase/util.h"
#include "ui/CocosGUI.h"
using std::to_string;

void LogMessage(const char*, ...);
bool ProcessEvents(int);
void WaitForCompletion(const firebase::FutureBase&, const char*);

std::string GenerateUid(std::size_t);

class MainMenuScene : public cocos2d::Layer, public cocos2d::TextFieldDelegate {
 public:
  // Builds a simple scene that uses the bottom left cordinate point as (0,0)
  // and can have sprites, labels and nodes added onto it.
  static cocos2d::Scene* createScene();
  virtual void MainMenuScene::update(float);
  virtual void MainMenuScene::onEnter();
  void MainMenuScene::updateUserRecord();
  void MainMenuScene::initializeUserRecord();
  // Initializes the instance of a Node and returns a boolean based on if it was
  // successful in doing so.
  virtual bool init();
  // Defines a create type for a specific type, in this case a Layer.
  CREATE_FUNC(MainMenuScene);
  cocos2d::DrawNode* auth_background;
  cocos2d::Node auth_node;
  cocos2d::Label* invalid_login_label;
  cocos2d::Label* user_record_label;
  cocos2d::TextFieldTTF* email_text_field;
  cocos2d::TextFieldTTF* password_text_field;
  // Defines the stage that the class is in.
  enum stageEnum {
    kAuthState,
    kGameMenuState,
    kWaitingAnonymousState,
    kWaitingSignUpState,
    kWaitingLoginState,
    kWaitingGameOutcome
  };
  // Defines the current stage
  stageEnum current_state = kAuthState;
  stageEnum previous_state = kAuthState;
  firebase::Future<firebase::auth::User*> user_result;
  std::string user_uid;
  int user_wins;
  int user_loses;
  int user_ties;
  firebase::auth::User* user;
  firebase::database::Database* database;
  ::firebase::auth::Auth* auth;
  firebase::database::DatabaseReference ref;
};

#endif  // TICTACTOE_DEMO_CLASSES_MAINMENU_SCENE_H_
