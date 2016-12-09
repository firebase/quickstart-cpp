// Copyright 2016 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#ifndef _WIN32
#include <unistd.h>
#endif  // !_WIN32

#ifdef _WIN32
#include <windows.h>
#endif  // _WIN32

#include "main.h"  // NOLINT

extern "C" int common_main(int argc, const char* argv[]);

static bool quit = false;

#ifdef _WIN32
static BOOL WINAPI SignalHandler(DWORD event) {
  if (!(event == CTRL_C_EVENT || event == CTRL_BREAK_EVENT)) {
    return FALSE;
  }
  quit = true;
  return TRUE;
}
#else
static void SignalHandler(int /* ignored */) { quit = true; }
#endif  // _WIN32

bool ProcessEvents(int msec) {
#ifdef _WIN32
  Sleep(msec);
#else
  usleep(msec * 1000);
#endif  // _WIN32
  return quit;
}

void LogMessage(const char* format, ...) {
  va_list list;
  va_start(list, format);
  vprintf(format, list);
  va_end(list);
  printf("\n");
  fflush(stdout);
}

WindowContext GetWindowContext() { return nullptr; }

int main(int argc, const char* argv[]) {
#ifdef _WIN32
  SetConsoleCtrlHandler((PHANDLER_ROUTINE)SignalHandler, TRUE);
#else
  signal(SIGINT, SignalHandler);
#endif  // _WIN32
  return common_main(argc, argv);
}
