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

#include <assert.h>

#include "firebase/app.h"
#include "firebase/remote_config.h"

// Thin OS abstraction layer.
#include "main.h"  // NOLINT

// Execute all methods of the C++ Remote Config API.
extern "C" int common_main(int argc, const char* argv[]) {
  namespace remote_config = ::firebase::remote_config;
  ::firebase::App* app;

  LogMessage("Initialize the Firebase Remote Config library");
  do {
#if defined(__ANDROID__)
    app = ::firebase::App::Create(::firebase::AppOptions(), GetJniEnv(),
                                  GetActivity());
#else
    app = ::firebase::App::Create(::firebase::AppOptions());
#endif  // defined(__ANDROID__)

    if (app == nullptr) {
      LogMessage("Couldn't create firebase app, try again.");
      // Wait a few moments, and try to create app again.
      ProcessEvents(1000);
    }
  } while (app == nullptr);

  LogMessage("Created the Firebase app %x",
             static_cast<int>(reinterpret_cast<intptr_t>(app)));
  remote_config::Initialize(*app);
  LogMessage("Initialized the Firebase Remote Config API");

  static const remote_config::ConfigKeyValue defaults[] = {
      {"TestBoolean", "True"},
      {"TestLong", "42"},
      {"TestDouble", "3.14"},
      {"TestString", "Hello World"},
      {"TestData", "abcde"}};
  size_t default_count = sizeof(defaults) / sizeof(defaults[0]);
  remote_config::SetDefaults(defaults, default_count);

  // The return values may not be the set defaults, if a fetch was previously
  // completed for the app that set them.
  {
    bool result = remote_config::GetBoolean("TestBoolean");
    LogMessage("Get TestBoolean %d", result ? 1 : 0);
  }
  {
    int64_t result = remote_config::GetLong("TestLong");
    LogMessage("Get TestLong %lld", result);
  }
  {
    double result = remote_config::GetDouble("TestDouble");
    LogMessage("Get TestDouble %f", result);
  }
  {
    std::string result = remote_config::GetString("TestString");
    LogMessage("Get TestString %s", result.c_str());
  }
  {
    std::vector<unsigned char> result = remote_config::GetData("TestData");
    for (size_t i = 0; i < result.size(); ++i) {
      const unsigned char value = result[i];
      LogMessage("TestData[%d] = 0x%02x (%c)", i, value, value);
    }
  }

  // Enable developer mode and verified it's enabled.
  // NOTE: Developer mode should not be enabled in production applications.
  remote_config::SetConfigSetting(remote_config::kConfigSettingDeveloperMode,
                                  "1");
  assert(*remote_config::GetConfigSetting(
              remote_config::kConfigSettingDeveloperMode)
              .c_str() == '1');

  LogMessage("Fetch...");
  auto future_result = remote_config::Fetch(0);
  while (future_result.Status() == firebase::kFutureStatusPending) {
    if (ProcessEvents(1000)) {
      break;
    }
  }

  if (future_result.Status() == firebase::kFutureStatusComplete) {
    LogMessage("Fetch Complete");
    bool activate_result = remote_config::ActivateFetched();
    LogMessage("ActivateFetched %s", activate_result ? "succeeded" : "failed");

    const remote_config::ConfigInfo& info = remote_config::GetInfo();
    LogMessage("Info last_fetch_time_ms=%d fetch_status=%d failure_reason=%d",
               static_cast<int>(info.fetch_time), info.last_fetch_status,
               info.last_fetch_failure_reason);

    // Print out the new values, which may be updated from the Fetch.
    {
      bool result = remote_config::GetBoolean("TestBoolean");
      LogMessage("Updated TestBoolean %d", result ? 1 : 0);
    }
    {
      int64_t result = remote_config::GetLong("TestLong");
      LogMessage("Updated TestLong %lld", result);
    }
    {
      double result = remote_config::GetDouble("TestDouble");
      LogMessage("Updated TestDouble %f", result);
    }
    {
      std::string result = remote_config::GetString("TestString");
      LogMessage("Updated TestString %s", result.c_str());
    }
    {
      std::vector<unsigned char> result = remote_config::GetData("TestData");
      for (size_t i = 0; i < result.size(); ++i) {
        const unsigned char value = result[i];
        LogMessage("TestData[%d] = 0x%02x (%c)", i, value, value);
      }
    }
  }
  // Release a handle to the future so we can shutdown the Remote Config API
  // when exiting the app.  Alternatively we could have placed future_result
  // in a scope different to our shutdown code below.
  future_result.Release();

  // Wait until the user wants to quit the app.
  while (!ProcessEvents(1000)) {
  }

  remote_config::Terminate();
  delete app;

  return 0;
}
