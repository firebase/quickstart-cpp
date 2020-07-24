Firebase Cocos2d-x TicTacToe Sample
==========================

Desktop cocos2d-x sample for the Firebase C++ SDK.

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

- Place the google-services.json you just acquired in the demos/TicTacToe/google_services/ directory.

- Navigate to the directory that you just cloned, step into demos/TicTacToe/ and run the setup script.
  ```
  python setup_demo.py 
  ```

- To debug with Visual Studio 2019, copy the google-services.json file into the tic_tac_toe_demo/build/ directory. Then open the .sln in that same directory in Visual Studio.

Support
-------

https://firebase.google.com/support/

License
-------

Copyright 2020 Google Inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.