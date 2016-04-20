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

#include "firebase/app.h"
#include "firebase/remote_config.h"

// Thin OS abstraction layer.
#include "main.h"  // NOLINT

::firebase::App* g_app;

// Execute all methods of the C++ Remote Config API.
extern "C" int common_main(int argc, const char* argv[]) {
  namespace remote_config = ::firebase::remote_config;

  LogMessage("Initialize the Firebase Remote Config library");
#if defined(__ANDROID__)
  g_app = ::firebase::App::Create(::firebase::AppOptions(FIREBASE_TESTAPP_NAME),
                                  GetJniEnv(), GetActivity());
#else
  g_app =
      ::firebase::App::Create(::firebase::AppOptions(FIREBASE_TESTAPP_NAME));
#endif  // defined(__ANDROID__)

  LogMessage("Created the Firebase app %x",
             static_cast<int>(reinterpret_cast<intptr_t>(g_app)));
  remote_config::Initialize(*g_app);
  LogMessage("Initialized the Firebase Remote Config API");

  std::map<const char*, const char*> defaults;
  bool test_boolean = true;
  defaults["TestBoolean"] = "True";
  int64_t test_long = 42;
  defaults["TestLong"] = "42";
  double test_double = 3.14;
  defaults["TestDouble"] = "3.14";
  std::string test_string("Hello World");
  defaults["TestString"] = "Hello World";
  static const unsigned char test_data_array[] = {'a', 'b', 'c', 'd', 'e'};
  std::vector<unsigned char> test_data(
      test_data_array, &test_data_array[sizeof(test_data_array)]);
  defaults["TestData"] = "abcde";
  remote_config::SetDefaults(defaults);

  {
    bool result = remote_config::GetBoolean("TestBoolean");
    LogMessage("Get TestBoolean %d", result ? 1 : 0);
    assert(result == test_boolean);
  }
  {
    int64_t result = remote_config::GetLong("TestLong");
    LogMessage("Get TestLong %lld", result);
    assert(result == test_long);
  }
  {
    double result = remote_config::GetDouble("TestDouble");
    LogMessage("Get TestDouble %f", result);
    assert(result == test_double);
  }
  {
    std::string result = remote_config::GetString("TestString");
    LogMessage("Get TestString %s", result.c_str());
    assert(result.compare(test_string) == 0);
  }
  {
    std::vector<unsigned char> result = remote_config::GetData("TestData");
    for (size_t i = 0; i < result.size(); ++i) {
      const unsigned char value = result[i];
      LogMessage("TestData[%d] = 0x%02x (%c)", i, value, value);
    }
    assert(result == test_data);
  }

  auto future_result = remote_config::Fetch();
  while (future_result.Status() == firebase::kFutureStatus_Pending) {
    ProcessEvents(10);
  }
  LogMessage("Done with Fetch");
  bool activate_result = remote_config::ActivateFetched();
  LogMessage("ActivateFetched %s", activate_result ? "succeeded" : "failed");

  // Print out the new values, which may be updated from the Fetch.
  {
    bool result = remote_config::GetBoolean("TestBoolean");
    LogMessage("Updated Boolean %d", result ? 1 : 0);
  }
  {
    int64_t result = remote_config::GetLong("TestLong");
    LogMessage("Updated Long %lld", result);
  }
  {
    double result = remote_config::GetDouble("TestDouble");
    LogMessage("Updated Double %f", result);
  }
  {
    std::string result = remote_config::GetString("TestString");
    LogMessage("Updated String %s", result.c_str());
  }
  {
    std::vector<unsigned char> result = remote_config::GetData("TestData");
    for (size_t i = 0; i < result.size(); ++i) {
      const unsigned char value = result[i];
      LogMessage("TestData[%d] = 0x%02x (%c)", i, value, value);
    }
  }

  // Wait until the user wants to quit the app.
  while (!ProcessEvents(1000)) {
  }

  return 0;
}
