// Copyright 2016 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <algorithm>
#include <ctime>
#include "firebase/app.h"
#include "firebase/auth.h"
#include "firebase/functions.h"
#include "firebase/future.h"
#include "firebase/util.h"

#include <math.h>

#include <map>
#include <vector>

// Thin OS abstraction layer.
#include "main.h" // NOLINT

// Wait for a Future to be completed. If the Future returns an error, it will
// be logged.
void WaitForCompletion(const firebase::FutureBase &future, const char *name)
{
  while (future.status() == firebase::kFutureStatusPending)
  {
    ProcessEvents(100);
  }
  if (future.status() != firebase::kFutureStatusComplete)
  {
    LogMessage("ERROR: %s returned an invalid result.", name);
  }
  else if (future.error() != 0)
  {
    LogMessage("ERROR: %s returned error %d: %s", name, future.error(),
           future.error_message());
  }
}

double GetDoubleVal(const firebase::Variant &val)
{
  if (val.is_double())
    return val.double_value();
  if (val.is_int64())
    return (double)val.int64_value();

  return 0.0;
}

bool CheckDoubleInt64(const firebase::Variant &lhs, const firebase::Variant &rhs)
{
  double left = GetDoubleVal(lhs);
  double right = GetDoubleVal(rhs);

  return fabs(left - right) < 0.01;
}

bool IsEqual(const firebase::Variant &lhs, const firebase::Variant &rhs)
{
  if (lhs.is_numeric() && rhs.is_numeric())
    return CheckDoubleInt64(lhs, rhs);
  else if (lhs.is_map() && rhs.is_map())
  {
    const auto &lhs_map = lhs.map();
    const auto &rhs_map = rhs.map();

    if (lhs_map.size() != rhs_map.size())
      return false;

    for (const auto &kv : lhs_map)
    {
      if (rhs_map.find(kv.first) == rhs_map.end())
        return false;

      if (!IsEqual(rhs_map.at(kv.first), kv.second))
        return false;
    }
    return true;
  }
  else if (lhs.is_vector() && rhs.is_vector())
  {
    const auto &lhs_arr = lhs.vector();
    const auto &rhs_arr = rhs.vector();

    if (lhs_arr.size() != rhs_arr.size())
      return false;

    for (size_t i = 0; i < lhs_arr.size(); ++i)
    {
      if (!IsEqual(lhs_arr[i], rhs_arr[i]))
        return false;
    }
  }
  else if (lhs == rhs)
    return true;
  return false;
}

void TestEcho(firebase::functions::HttpsCallableReference &server_func, const firebase::Variant &test_var, const char *test_name)
{
  LogMessage("Starting test [%s]...", test_name);
  auto call = server_func.Call(test_var);
  WaitForCompletion(call, test_name);
  if (call.error() != firebase::functions::kErrorNone)
  {
    LogMessage("[%s] error: %s", test_name, call.error_message());
    return;
  }

  if (call.result() == nullptr)
  {
    LogMessage("[%s] error: missing return value", test_name);
    return;
  }

  auto result_var = call.result()->data();

  if (IsEqual(test_var, result_var))
    LogMessage("[%s] : success", test_name);
  else
    LogMessage("[%s] error: Return value is not equal", test_name);
}

extern "C" int common_main(int argc, const char *argv[])
{
  ::firebase::App *app;

#if defined(__ANDROID__)
  app = ::firebase::App::Create(GetJniEnv(), GetActivity());
#else
  app = ::firebase::App::Create();
#endif // defined(__ANDROID__)

  LogMessage("Initialized Firebase App.");

  auto functions = firebase::functions::Functions::GetInstance(app);

  if (functions != nullptr)
  {
    LogMessage("Initialized Functions.");

    // Setup data to send
    auto bool_var = firebase::Variant((bool)true);
    auto float_var = firebase::Variant((double)5.0f);
    auto int_var = firebase::Variant((int)42);
    auto int64_var = firebase::Variant((int64_t)99);

    std::map<firebase::Variant, firebase::Variant> map_var;
    map_var["test_bool"] = bool_var;
    map_var["test_float"] = float_var;
    map_var["test_int"] = int_var;
    map_var["test_int64"] = int64_var;

    std::vector<firebase::Variant> arr_var;
    arr_var.push_back(map_var);
    arr_var.push_back(bool_var);
    arr_var.push_back(float_var);
    arr_var.push_back(int_var);
    arr_var.push_back(int64_var);

    // The name of the Cloud Function on the server to call
    const char *remote_func_name = "echoBody"; // this method echos what is sent

    // The Reference to the remote function
    auto func_ref = functions->GetHttpsCallable(remote_func_name);
    if (func_ref.is_valid())
    {
      LogMessage("Got reference to function: %s", remote_func_name);

      TestEcho(func_ref, bool_var, "bool");
      TestEcho(func_ref, float_var, "float");
      TestEcho(func_ref, int_var, "int");
      TestEcho(func_ref, int64_var, "int64");
      TestEcho(func_ref, map_var, "map");
      TestEcho(func_ref, arr_var, "array");
    }
    else
      LogMessage("ERROR: %s is not a valid function", remote_func_name);
  }
  else
    LogMessage("ERROR: starting functions");

  LogMessage("Shutdown Functions.");
  delete functions;

  LogMessage("Shutdown Firebase App.");
  delete app;

  // Wait until the user wants to quit the app.
  while (!ProcessEvents(1000))
  {
  }

  return 0;
}
