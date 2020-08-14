Firebase Cocos2d-x TicTacToe Sample 
==========================

Desktop cocos2d-x sample using Firebase Real-Time Database and Firebase Authentication.

Introduction
------------

- [Read more about Firebase](https://firebase.google.com/docs)

Prerequisites
-------------
- [Cmake](https://cmake.org/download/) (Minimum 3.6)

- [Python 2.7](https://www.python.org/download/releases/2.7/) 

- The latest version [of Cocos2d-x](https://cocos2d-x.org/download) (cocos environment variable must be added to PATH) 

-  [Firebase CPP SDK](https://cocos2d-x.org/download) (Must be add to path under FIREBASE_CPP_SDK_DIR) 

-  Windows: ([Visual Studio 2019](https://visualstudio.microsoft.com/downloads/))

Getting Started
---------------

- Clone the firebase cpp quickstart GitHub repo.
  ```
  git clone https://github.com/firebase/quickstart-cpp.git
  ```

### Windows
- Follow the steps in
  [Set up your app in Firebase console](https://firebase.google.com/docs/cpp/setup?platform=android#desktop-workflow).

- Enable Anonymous Auth & Email/Password sign-in:
    a. In the [Firebase console](https://console.firebase.google.com/), open the Auth section.
    b. On the Sign in method tab, enable the Anonymous Auth & Email/password sign-in method and click Save.

- Place the google-services.json you just acquired in the demos/TicTacToe/google_services/ directory.

- Navigate to the directory that you just cloned, step into demos/TicTacToe/ and run the setup script.
  ```
  python setup_demo.py 
  ```

- To debug with Visual Studio 2019, copy the google-services.json file into the tic_tac_toe_demo/build/ directory. Then open the .sln in that same directory in Visual Studio.

### How To Play
---------------

- After you have successfully run the setup_demo.py script, you can then launch the game by running the run_demo.py script. In order to test the multiplayer functionality, open up another terminal and launch a second instant of the game by running the run_demo.py script again.  
  ```
  python run_demo.py 
  ```

- Using the sign-up button or login button will link your record (Wins-Losses-Ties) to your account. The skip login button will not save your record between sessions.

- From the main menu screen, you can either create a game or join using a game code (displayed in the top-left corner of the game board). If you are running this demo yourself, have on instance create the game and join with your second game instance.

- After finishing the game, you will notice the record will update on the main menu to reflect the result of your most recent game.



## Future Improvements
---------------

### Accessability & Build Process

#### 1. Build on MacOS.
    By making slight modifications to the setup_demo.py script, making cmake generator an argument, the application should build and run on MacOS.


#### 2. Build on linux.
    By making slight modifications to the setup_demo.py script, making cmake generator an argument, the application should build and run on linux.


#### 3. Build on Android

#### 4. Build on iOS

#### 5. Cmake optimizations. 
    - Use FetchContent to grab the latest Firebase SDK.
    - Attempt to get around Python 2.7 dependency, this may already work as the scripts use cmake.
    - Attempt to use FetchContent to grab Cocos2d-x library and grab the files currently being produced by the 'cocos new' command.


### Features

#### 1. Add a replay system.
    This will require changing the game_data documents to keep track of the entire game, oppose to the last_move as it currently does.


#### 2. Add animations and sounds on actions/scene changes.


#### 3. Text field improvements. (Potentially swap to EditBox as this may be better supported)
    - If the Tab key is pressed inside a text field, it should attempt to move to then next text field.
    - When a player pressed inside a text field will existing text, it should place the cursor in the selected location.


#### 4. Add Additional Authentication providers.

### Bugs

#### 1. Close application callback.
    If the player closes the application, this should trigger a callback to update relevant Firebase feature data.

#### Code Health

### 1. Remove all blocking statements (WaitForCompletion()).
    Removes the WaitForCompletion() function. Instead, rely on the state management to handle future requests.

Support
-------

https://firebase.google.com/support/

License
-------

Copyright 2020 Google, Inc.

Licensed to the Apache Software Foundation (ASF) under one or more contributor license agreements. See the NOTICE file distributed with this work for additional information regarding copyright ownership. The ASF licenses this file to you under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License..