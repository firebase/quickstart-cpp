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
#ifndef TICTACTOE_DEMO_CLASSES_UTIL_H_
#define TICTACTOE_DEMO_CLASSES_UTIL_H_

#include <random>
#include <string>

#include "cocos2d.h"
#include "firebase/auth.h"
#include "firebase/database.h"
#include "firebase/future.h"
#include "firebase/util.h"
#include "ui/CocosGUI.h"

// inputs: const char* as the message that will be displayed.
//
// Logs the message to the user through the console.
void LogMessage(const char*, ...);

// inputs: integer for number of miliseconds.
//
// Acts as a blocking statement to wait for the specified time.
void ProcessEvents(int);

// inputs: FutureBase waiting to be completed and message describing the future
// action.
//
// Spins on ProcessEvents() while the FutureBase's status is pending, which it
// will then break out and LogMessage() notifying the FutureBase's status is
// completed.
void WaitForCompletion(const firebase::FutureBase&, const char*);

// inputs: size_t integer specifying the length of the uid.
//
// Generates a unique string based on the length passed in, grabbing the allowed
// characters from the character string defined inside the function.
std::string GenerateUid(std::size_t);

#endif  // TICTACTOE_DEMO_CLASSES_UTIL_H_
