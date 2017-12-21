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
#include "firebase/util.h"

// Thin OS abstraction layer.
#include "main.h"  // NOLINT

// Convert remote_config::ValueSource to a string.
const char* ValueSourceToString(firebase::remote_config::ValueSource source) {
  static const char* kSourceToString[] = {
    "Static",   // kValueSourceStaticValue
    "Remote",   // kValueSourceRemoteValue
    "Default",  // kValueSourceDefaultValue
  };
  return kSourceToString[source];
}

// Execute all methods of the C++ Remote Config API.
extern "C" int common_main(int argc, const char* argv[]) {
  namespace remote_config = ::firebase::remote_config;
  ::firebase::App* app;

  // Initialization

  LogMessage("Initialize the Firebase Remote Config library");
#if defined(__ANDROID__)
  app = ::firebase::App::Create(GetJniEnv(), GetActivity());
#else
  app = ::firebase::App::Create();
#endif  // defined(__ANDROID__)

  LogMessage("Created the Firebase app %x",
             static_cast<int>(reinterpret_cast<intptr_t>(app)));

  ::firebase::ModuleInitializer initializer;
  initializer.Initialize(app, nullptr, [](::firebase::App* app, void*) {
    LogMessage("Try to initialize Remote Config");
    return ::firebase::remote_config::Initialize(*app);
  });
  while (initializer.InitializeLastResult().status() !=
         firebase::kFutureStatusComplete) {
    if (ProcessEvents(100)) return 1;  // exit if requested
  }
  if (initializer.InitializeLastResult().error() != 0) {
    LogMessage("Failed to initialize Firebase Remote Config: %s",
               initializer.InitializeLastResult().error_message());
    ProcessEvents(2000);
    return 1;
  }

  LogMessage("Initialized the Firebase Remote Config API");

  // Initialization Complete
  // Set Defaults, and test them
  static const unsigned char kBinaryDefaults[] = {6, 0, 0, 6, 7, 3};

  static const remote_config::ConfigKeyValueVariant defaults[] = {
      {"TestBoolean", "True"},
      {"TestLong", 42},
      {"TestDouble", 3.14},
      {"TestString", "Hello World"},
      {"TestData", firebase::Variant::FromStaticBlob(kBinaryDefaults,
                                                     sizeof(kBinaryDefaults))},
      {"TestDefaultOnly", "Default value that won't be overridden"}};
  size_t default_count = sizeof(defaults) / sizeof(defaults[0]);
  remote_config::SetDefaults(defaults, default_count);

  // The return values may not be the set defaults, if a fetch was previously
  // completed for the app that set them.
  remote_config::ValueInfo value_info;
  {
    bool result = remote_config::GetBoolean("TestBoolean", &value_info);
    LogMessage("Get TestBoolean %d %s", result ? 1 : 0,
               ValueSourceToString(value_info.source));
  }
  {
    int64_t result = remote_config::GetLong("TestLong", &value_info);
    LogMessage("Get TestLong %lld %s", result,
               ValueSourceToString(value_info.source));
  }
  {
    double result = remote_config::GetDouble("TestDouble", &value_info);
    LogMessage("Get TestDouble %f %s", result,
               ValueSourceToString(value_info.source));
  }
  {
    std::string result = remote_config::GetString("TestString", &value_info);
    LogMessage("Get TestString \"%s\" %s", result.c_str(),
               ValueSourceToString(value_info.source));
  }
  {
    std::vector<unsigned char> result = remote_config::GetData("TestData");
    for (size_t i = 0; i < result.size(); ++i) {
      const unsigned char value = result[i];
      LogMessage("TestData[%d] = 0x%02x", i, value);
    }
  }
  {
    std::string result = remote_config::GetString("TestDefaultOnly",
                                                  &value_info);
    LogMessage("Get TestDefaultOnly \"%s\" %s", result.c_str(),
               ValueSourceToString(value_info.source));
  }
  {
    std::string result = remote_config::GetString("TestNotSet",
                                                  &value_info);
    LogMessage("Get TestNotSet \"%s\" %s", result.c_str(),
               ValueSourceToString(value_info.source));
  }

  // Test the existence of the keys by name.
  {
    // Print out the keys with default values.
    std::vector<std::string> keys = remote_config::GetKeys();
    LogMessage("GetKeys:");
    for (auto s = keys.begin(); s != keys.end(); ++s) {
      LogMessage("  %s", s->c_str());
    }
    keys = remote_config::GetKeysByPrefix("TestD");
    LogMessage("GetKeysByPrefix(\"TestD\"):");
    for (auto s = keys.begin(); s != keys.end(); ++s) {
      LogMessage("  %s", s->c_str());
    }
  }
  // Enable developer mode and verified it's enabled.
  // NOTE: Developer mode should not be enabled in production applications.
  remote_config::SetConfigSetting(remote_config::kConfigSettingDeveloperMode,
                                  "1");
  if ((*remote_config::GetConfigSetting(
            remote_config::kConfigSettingDeveloperMode)
            .c_str()) != '1') {
    LogMessage("Failed to enable developer mode");
  }

  // Test Fetch...
  LogMessage("Fetch...");
  auto future_result = remote_config::Fetch(0);
  while (future_result.status() == firebase::kFutureStatusPending) {
    if (ProcessEvents(1000)) {
      break;
    }
  }

  if (future_result.status() == firebase::kFutureStatusComplete) {
    LogMessage("Fetch Complete");
    bool activate_result = remote_config::ActivateFetched();
    LogMessage("ActivateFetched %s", activate_result ? "succeeded" : "failed");

    const remote_config::ConfigInfo& info = remote_config::GetInfo();
    LogMessage(
        "Info last_fetch_time_ms=%d (year=%.2f) fetch_status=%d "
        "failure_reason=%d throttled_end_time=%d",
        static_cast<int>(info.fetch_time),
        1970.0f + static_cast<float>(info.fetch_time) /
                      (1000.0f * 60.0f * 60.0f * 24.0f * 365.0f),
        info.last_fetch_status, info.last_fetch_failure_reason,
        info.throttled_end_time);

    // Print out the new values, which may be updated from the Fetch.
    {
      bool result = remote_config::GetBoolean("TestBoolean", &value_info);
      LogMessage("Updated TestBoolean %d %s", result ? 1 : 0,
                 ValueSourceToString(value_info.source));
    }
    {
      int64_t result = remote_config::GetLong("TestLong", &value_info);
      LogMessage("Updated TestLong %lld %s", result,
                 ValueSourceToString(value_info.source));
    }
    {
      double result = remote_config::GetDouble("TestDouble", &value_info);
      LogMessage("Updated TestDouble %f %s", result,
                 ValueSourceToString(value_info.source));
    }
    {
      std::string result = remote_config::GetString("TestString", &value_info);
      LogMessage("Updated TestString \"%s\" %s", result.c_str(),
                 ValueSourceToString(value_info.source));
    }
    {
      std::vector<unsigned char> result = remote_config::GetData("TestData");
      for (size_t i = 0; i < result.size(); ++i) {
        const unsigned char value = result[i];
        LogMessage("TestData[%d] = 0x%02x", i, value);
      }
    }
    {
      std::string result = remote_config::GetString("TestDefaultOnly",
                                                    &value_info);
      LogMessage("Get TestDefaultOnly \"%s\" %s", result.c_str(),
                 ValueSourceToString(value_info.source));
    }
    {
      std::string result = remote_config::GetString("TestNotSet",
                                                    &value_info);
      LogMessage("Get TestNotSet \"%s\" %s", result.c_str(),
                 ValueSourceToString(value_info.source));
    }
    {
      // Print out the keys that are now tied to data
      std::vector<std::string> keys = remote_config::GetKeys();
      LogMessage("GetKeys:");
      for (auto s = keys.begin(); s != keys.end(); ++s) {
        LogMessage("  %s", s->c_str());
      }
      keys = remote_config::GetKeysByPrefix("TestD");
      LogMessage("GetKeysByPrefix(\"TestD\"):");
      for (auto s = keys.begin(); s != keys.end(); ++s) {
        LogMessage("  %s", s->c_str());
      }
    }
  } else {
    LogMessage("Fetch Incomplete");
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
