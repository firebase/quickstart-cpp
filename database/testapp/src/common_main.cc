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

#include <algorithm>
#include <ctime>
#include "firebase/app.h"
#include "firebase/auth.h"
#include "firebase/database.h"
#include "firebase/future.h"
#include "firebase/util.h"

// Thin OS abstraction layer.
#include "main.h"  // NOLINT

// An example of a ValueListener object. This specific version will
// simply log every value it sees, and store them in a list so we can
// confirm that all values were received.
class SampleValueListener : public firebase::database::ValueListener {
 public:
  void OnValueChanged(
      const firebase::database::DataSnapshot& snapshot) override {
    LogMessage("  ValueListener.OnValueChanged(%s)",
               snapshot.value().AsString().string_value());
    last_seen_value_ = snapshot.value();
    seen_values_.push_back(snapshot.value());
  }
  void OnCancelled(const firebase::database::Error& error_code,
                   const char* error_message) override {
    LogMessage("ERROR: SampleValueListener canceled: %d: %s", error_code,
               error_message);
  }
  const firebase::Variant& last_seen_value() { return last_seen_value_; }
  bool seen_value(const firebase::Variant& value) {
    return std::find(seen_values_.begin(), seen_values_.end(), value) !=
           seen_values_.end();
  }
  size_t num_seen_values() { return seen_values_.size(); }

 private:
  firebase::Variant last_seen_value_;
  std::vector<firebase::Variant> seen_values_;
};

// An example ChildListener class.
class SampleChildListener : public firebase::database::ChildListener {
 public:
  void OnChildAdded(const firebase::database::DataSnapshot& snapshot,
                    const char* previous_sibling) override {
    LogMessage("  ChildListener.OnChildAdded(%s)", snapshot.key());
    events_.push_back(std::string("added ") + snapshot.key());
  }
  void OnChildChanged(const firebase::database::DataSnapshot& snapshot,
                      const char* previous_sibling) override {
    LogMessage("  ChildListener.OnChildChanged(%s)", snapshot.key());
    events_.push_back(std::string("changed ") + snapshot.key());
  }
  void OnChildMoved(const firebase::database::DataSnapshot& snapshot,
                    const char* previous_sibling) override {
    LogMessage("  ChildListener.OnChildMoved(%s)", snapshot.key());
    events_.push_back(std::string("moved ") + snapshot.key());
  }
  void OnChildRemoved(
      const firebase::database::DataSnapshot& snapshot) override {
    LogMessage("  ChildListener.OnChildRemoved(%s)", snapshot.key());
    events_.push_back(std::string("removed ") + snapshot.key());
  }
  void OnCancelled(const firebase::database::Error& error_code,
                   const char* error_message) override {
    LogMessage("ERROR: SampleChildListener canceled: %d: %s", error_code,
               error_message);
  }

  // Get the total number of Child events this listener saw.
  size_t total_events() { return events_.size(); }

  // Get the number of times this event was seen.
  int num_events(const std::string& event) {
    int count = 0;
    for (int i = 0; i < events_.size(); i++) {
      if (events_[i] == event) {
        count++;
      }
    }
    return count;
  }

 public:
  // Vector of strings defining the events we saw, in order.
  std::vector<std::string> events_;
};

// A ValueListener that expects a specific value to be set.
class ExpectValueListener : public firebase::database::ValueListener {
 public:
  explicit ExpectValueListener(firebase::Variant wait_value)
      : wait_value_(wait_value.AsString()), got_value_(false) {}
  void OnValueChanged(
      const firebase::database::DataSnapshot& snapshot) override {
    if (snapshot.value().AsString() == wait_value_) {
      got_value_ = true;
    } else {
      LogMessage(
          "FAILURE: ExpectValueListener did not receive the expected result.");
    }
  }
  void OnCancelled(const firebase::database::Error& error_code,
                   const char* error_message) override {
    LogMessage("ERROR: ExpectValueListener canceled: %d: %s", error_code,
               error_message);
  }

  bool got_value() { return got_value_; }

 private:
  firebase::Variant wait_value_;
  bool got_value_;
};

// Wait for a Future to be completed. If the Future returns an error, it will
// be logged.
void WaitForCompletion(const firebase::FutureBase& future, const char* name) {
  while (future.status() == firebase::kFutureStatusPending) {
    ProcessEvents(100);
  }
  if (future.status() != firebase::kFutureStatusComplete) {
    LogMessage("ERROR: %s returned an invalid result.", name);
  } else if (future.error() != 0) {
    LogMessage("ERROR: %s returned error %d: %s", name, future.error(),
               future.error_message());
  }
}

extern "C" int common_main(int argc, const char* argv[]) {
  ::firebase::App* app;

#if defined(__ANDROID__)
  app = ::firebase::App::Create(GetJniEnv(), GetActivity());
#else
  app = ::firebase::App::Create();
#endif  // defined(__ANDROID__)

  LogMessage("Initialized Firebase App.");

  LogMessage("Initialize Firebase Auth and Firebase Database.");

  // Use ModuleInitializer to initialize both Auth and Database, ensuring no
  // dependencies are missing.
  ::firebase::database::Database* database = nullptr;
  ::firebase::auth::Auth* auth = nullptr;
  void* initialize_targets[] = {&auth, &database};

  const firebase::ModuleInitializer::InitializerFn initializers[] = {
      [](::firebase::App* app, void* data) {
        LogMessage("Attempt to initialize Firebase Auth.");
        void** targets = reinterpret_cast<void**>(data);
        ::firebase::InitResult result;
        *reinterpret_cast<::firebase::auth::Auth**>(targets[0]) =
            ::firebase::auth::Auth::GetAuth(app, &result);
        return result;
      },
      [](::firebase::App* app, void* data) {
        LogMessage("Attempt to initialize Firebase Database.");
        void** targets = reinterpret_cast<void**>(data);
        ::firebase::InitResult result;
        *reinterpret_cast<::firebase::database::Database**>(targets[1]) =
            ::firebase::database::Database::GetInstance(app, &result);
        return result;
      }};

  ::firebase::ModuleInitializer initializer;
  initializer.Initialize(app, initialize_targets, initializers,
                         sizeof(initializers) / sizeof(initializers[0]));

  WaitForCompletion(initializer.InitializeLastResult(), "Initialize");

  if (initializer.InitializeLastResult().error() != 0) {
    LogMessage("Failed to initialize Firebase libraries: %s",
               initializer.InitializeLastResult().error_message());
    ProcessEvents(2000);
    return 1;
  }
  LogMessage("Successfully initialized Firebase Auth and Firebase Database.");

  database->set_persistence_enabled(true);

  // Sign in using Auth before accessing the database.
  // The default Database permissions allow anonymous users access. This will
  // work as long as your project's Authentication permissions allow anonymous
  // signin.
  {
    firebase::Future<firebase::auth::User*> sign_in_future =
        auth->SignInAnonymously();
    WaitForCompletion(sign_in_future, "SignInAnonymously");
    if (sign_in_future.error() == firebase::auth::kAuthErrorNone) {
      LogMessage("Auth: Signed in anonymously.");
    } else {
      LogMessage("ERROR: Could not sign in anonymously. Error %d: %s",
                 sign_in_future.error(), sign_in_future.error_message());
      LogMessage(
          "  Ensure your application has the Anonymous sign-in provider "
          "enabled in Firebase Console.");
      LogMessage(
          "  Attempting to connect to the database anyway. This may fail "
          "depending on the security settings.");
    }
  }

  std::string saved_url;  // persists across connections

  // Create a unique child in the database that we can run our tests in.
  firebase::database::DatabaseReference ref;
  ref = database->GetReference("test_app_data").PushChild();

  saved_url = ref.url();
  LogMessage("URL: %s", saved_url.c_str());

  // Set and Get some simple fields. This will set a string, integer, double,
  // bool, and current timestamp, and then read them back from the database to
  // confirm that they were set. Then it will remove the string value.
  {
    const char* kSimpleString = "Some simple string";
    const int kSimpleInt = 2;
    const int kSimplePriority = 100;
    const double kSimpleDouble = 3.4;
    const bool kSimpleBool = true;

    {
      LogMessage("TEST: Set simple values.");
      firebase::Future<void> f1 =
          ref.Child("Simple").Child("String").SetValue(kSimpleString);
      firebase::Future<void> f2 =
          ref.Child("Simple").Child("Int").SetValue(kSimpleInt);
      firebase::Future<void> f3 =
          ref.Child("Simple").Child("Double").SetValue(kSimpleDouble);
      firebase::Future<void> f4 =
          ref.Child("Simple").Child("Bool").SetValue(kSimpleBool);
      firebase::Future<void> f5 =
          ref.Child("Simple")
              .Child("Timestamp")
              .SetValue(firebase::database::ServerTimestamp());
      firebase::Future<void> f6 =
          ref.Child("Simple")
              .Child("IntAndPriority")
              .SetValueAndPriority(kSimpleInt, kSimplePriority);
      WaitForCompletion(f1, "SetSimpleString");
      WaitForCompletion(f2, "SetSimpleInt");
      WaitForCompletion(f3, "SetSimpleDouble");
      WaitForCompletion(f4, "SetSimpleBool");
      WaitForCompletion(f5, "SetSimpleTimestamp");
      WaitForCompletion(f6, "SetSimpleIntAndPriority");
      if (f1.error() != firebase::database::kErrorNone ||
          f2.error() != firebase::database::kErrorNone ||
          f3.error() != firebase::database::kErrorNone ||
          f4.error() != firebase::database::kErrorNone ||
          f5.error() != firebase::database::kErrorNone ||
          f6.error() != firebase::database::kErrorNone) {
        LogMessage("ERROR: Set simple values failed.");
        LogMessage("  String: Error %d: %s", f1.error(), f1.error_message());
        LogMessage("  Int: Error %d: %s", f2.error(), f2.error_message());
        LogMessage("  Double: Error %d: %s", f3.error(), f3.error_message());
        LogMessage("  Bool: Error %d: %s", f4.error(), f4.error_message());
        LogMessage("  Timestamp: Error %d: %s", f5.error(), f5.error_message());
        LogMessage("  Int and Priority: Error %d: %s", f6.error(),
                   f6.error_message());
      } else {
        LogMessage("SUCCESS: Set simple values.");
      }
    }
    // Get the values that we just set, and confirm that they match what we
    // set them to.
    {
      LogMessage("TEST: Get simple values.");
      firebase::Future<firebase::database::DataSnapshot> f1 =
          ref.Child("Simple").Child("String").GetValue();
      firebase::Future<firebase::database::DataSnapshot> f2 =
          ref.Child("Simple").Child("Int").GetValue();
      firebase::Future<firebase::database::DataSnapshot> f3 =
          ref.Child("Simple").Child("Double").GetValue();
      firebase::Future<firebase::database::DataSnapshot> f4 =
          ref.Child("Simple").Child("Bool").GetValue();
      firebase::Future<firebase::database::DataSnapshot> f5 =
          ref.Child("Simple").Child("Timestamp").GetValue();
      firebase::Future<firebase::database::DataSnapshot> f6 =
          ref.Child("Simple").Child("IntAndPriority").GetValue();
      WaitForCompletion(f1, "GetSimpleString");
      WaitForCompletion(f2, "GetSimpleInt");
      WaitForCompletion(f3, "GetSimpleDouble");
      WaitForCompletion(f4, "GetSimpleBool");
      WaitForCompletion(f5, "GetSimpleTimestamp");
      WaitForCompletion(f6, "GetSimpleIntAndPriority");

      if (f1.error() == firebase::database::kErrorNone &&
          f2.error() == firebase::database::kErrorNone &&
          f3.error() == firebase::database::kErrorNone &&
          f4.error() == firebase::database::kErrorNone &&
          f5.error() == firebase::database::kErrorNone &&
          f6.error() == firebase::database::kErrorNone) {
        // Get the current time to compare to the Timestamp.
        int64_t current_time_milliseconds =
            static_cast<int64_t>(time(nullptr)) * 1000L;
        int64_t time_difference = f5.result()->value().AsInt64().int64_value() -
                                  current_time_milliseconds;
        // As long as our timestamp is within 15 minutes, it's correct enough
        // for our purposes.
        const int64_t kAllowedTimeDifferenceMilliseconds = 1000L * 60L * 15L;

        if (f1.result()->value().AsString() != kSimpleString ||
            f2.result()->value().AsInt64() != kSimpleInt ||
            f3.result()->value().AsDouble() != kSimpleDouble ||
            f4.result()->value().AsBool() != kSimpleBool ||
            f6.result()->value().AsInt64() != kSimpleInt ||
            f6.result()->priority().AsInt64() != kSimplePriority ||
            time_difference > kAllowedTimeDifferenceMilliseconds ||
            time_difference < -kAllowedTimeDifferenceMilliseconds) {
          LogMessage("ERROR: Get simple values failed, values did not match.");
          LogMessage("  String: Got \"%s\", expected \"%s\"",
                     f1.result()->value().string_value(), kSimpleString);
          LogMessage("  Int: Got %lld, expected %d",
                     f2.result()->value().AsInt64().int64_value(), kSimpleInt);
          LogMessage("  Double: Got %lf, expected %lf",
                     f3.result()->value().AsDouble().double_value(),
                     kSimpleDouble);
          LogMessage(
              "  Bool: Got %s, expected %s",
              f4.result()->value().AsBool().bool_value() ? "true" : "false",
              kSimpleBool ? "true" : "false");
          LogMessage("  Timestamp: Got %lld, expected something near %lld",
                     f5.result()->value().AsInt64().int64_value(),
                     current_time_milliseconds);
          LogMessage(
              "  IntAndPriority: Got {.value:%lld,.priority:%lld}, expected "
              "{.value:%d,.priority:%d}",
              f6.result()->value().AsInt64().int64_value(),
              f6.result()->priority().AsInt64().int64_value(), kSimpleInt,
              kSimplePriority);

        } else {
          LogMessage("SUCCESS: Get simple values.");
        }
      } else {
        LogMessage("ERROR: Get simple values failed.");
      }

      // Try removing one value.
      {
        LogMessage("TEST: Removing a value.");
        WaitForCompletion(ref.Child("Simple").Child("String").RemoveValue(),
                          "RemoveSimpleString");
        firebase::Future<firebase::database::DataSnapshot> future =
            ref.Child("Simple").Child("String").GetValue();
        WaitForCompletion(future, "GetRemovedSimpleString");
        if (future.error() == firebase::database::kErrorNone &&
            future.result()->value().is_null()) {
          LogMessage("SUCCESS: Value was removed.");
        } else {
          LogMessage("ERROR: Value was not removed.");
        }
      }
    }
  }

#if defined(__ANDROID__) || TARGET_OS_IPHONE
  // Actually shut down the realtime database, and restart it, to make sure
  // that persistence persists across database object instances.
  {
    // Write a value that we can test for.
    const char* kPersistenceString = "Persistence Test!";
    WaitForCompletion(ref.Child("PersistenceTest").SetValue(kPersistenceString),
                      "SetPersistenceTestValue");

    LogMessage("Destroying database object.");
    delete database;
    LogMessage("Recreating database object.");
    database = ::firebase::database::Database::GetInstance(app);

    // Offline mode.  If persistence works, we should still be able to fetch
    // our value even though we're offline.

    database->GoOffline();
    ref = database->GetReferenceFromUrl(saved_url.c_str());

    {
      LogMessage(
          "TEST: Fetching the value while offline via AddValueListener.");
      ExpectValueListener* listener =
          new ExpectValueListener(kPersistenceString);
      ref.Child("PersistenceTest").AddValueListener(listener);

      while (!listener->got_value()) {
        ProcessEvents(100);
      }
      delete listener;
      listener = nullptr;
    }

    {
      LogMessage("TEST: Fetching the value while offline via GetValue.");
      firebase::Future<firebase::database::DataSnapshot> value_future =
          ref.Child("PersistenceTest").GetValue();

      WaitForCompletion(value_future, "GetValue");

      const firebase::database::DataSnapshot& result = *value_future.result();

      if (value_future.error() == firebase::database::kErrorNone) {
        if (result.value().AsString() == kPersistenceString) {
          LogMessage("SUCCESS: GetValue returned the correct value.");
        } else {
          LogMessage("FAILURE: GetValue returned an incorrect value.");
        }
      } else {
        LogMessage("FAILURE: GetValue Future returned an error.");
      }
    }

    LogMessage("Going back online.");
    database->GoOnline();
  }
#endif  // defined(__ANDROID__) || TARGET_OS_IPHONE

  // Test running a transaction. This will call RunTransaction and set
  // some values, including incrementing the player's score.
  {
    firebase::Future<firebase::database::DataSnapshot> transaction_future;
    static const int kInitialScore = 500;
    static const int kAddedScore = 100;
    LogMessage("TEST: Run transaction.");
    // Set an initial score of 500 points.
    WaitForCompletion(ref.Child("TransactionResult")
                          .Child("player_score")
                          .SetValue(kInitialScore),
                      "SetInitialScoreValue");
    // The transaction will set the player's item and class, and increment
    // their score by 100 points.
    int score_delta = 100;
    transaction_future =
        ref.Child("TransactionResult")
            .RunTransaction(
                [](firebase::database::MutableData* data,
                   void* score_delta_void) {
                  LogMessage("  Transaction function executing.");
                  data->Child("player_item").set_value("Fire sword");
                  data->Child("player_class").set_value("Warrior");
                  // Increment the current score by 100.
                  int64_t score = data->Child("player_score")
                                      .value()
                                      .AsInt64()
                                      .int64_value();
                  data->Child("player_score")
                      .set_value(score +
                                 *reinterpret_cast<int*>(score_delta_void));
                  return firebase::database::kTransactionResultSuccess;
                },
                &score_delta);
    WaitForCompletion(transaction_future, "RunTransaction");

    // Check whether the transaction succeeded, was aborted, or failed with an
    // error.
    if (transaction_future.error() == firebase::database::kErrorNone) {
      LogMessage("SUCCESS: Transaction committed.");
    } else if (transaction_future.error() ==
               firebase::database::kErrorTransactionAbortedByUser) {
      LogMessage("ERROR: Transaction was aborted.");
    } else {
      LogMessage("ERROR: Transaction returned error %d: %s",
                 transaction_future.error(),
                 transaction_future.error_message());
    }

    // If the transaction succeeded, let's read back the values that were
    // written to confirm they match.
    if (transaction_future.error() == firebase::database::kErrorNone) {
      LogMessage("TEST: Test reading transaction results.");

      firebase::Future<firebase::database::DataSnapshot> read_future =
          ref.Child("TransactionResult").GetValue();
      WaitForCompletion(read_future, "ReadTransactionResults");
      if (read_future.error() != firebase::database::kErrorNone) {
        LogMessage("ERROR: Error %d reading transaction results: %s",
                   read_future.error(), read_future.error_message());
      } else {
        const firebase::database::DataSnapshot& result = *read_future.result();
        if (result.children_count() == 3 && result.HasChild("player_item") &&
            result.Child("player_item").value() == "Fire sword" &&
            result.HasChild("player_class") &&
            result.Child("player_class").value() == "Warrior" &&
            result.HasChild("player_score") &&
            result.Child("player_score").value().AsInt64() ==
                kInitialScore + kAddedScore) {
          if (result.value() != transaction_future.result()->value()) {
            LogMessage(
                "ERROR: Transaction snapshot did not match newly read data.");
          } else {
            LogMessage("SUCCESS: Transaction test succeeded.");
          }
        } else {
          LogMessage("ERROR: Transaction result was incorrect.");
        }
      }
    }
  }

  // Set up a map of values that we will put into the database, then modify.
  std::map<std::string, int> sample_values;
  sample_values.insert(std::make_pair("Apple", 1));
  sample_values.insert(std::make_pair("Banana", 2));
  sample_values.insert(std::make_pair("Cranberry", 3));
  sample_values.insert(std::make_pair("Durian", 4));
  sample_values.insert(std::make_pair("Eggplant", 5));

  // Run UpdateChildren, specifying some existing children (which will be
  // modified), some children with a value of null (which will be removed),
  // and some new children (which will be added).
  {
    LogMessage("TEST: UpdateChildren.");

    WaitForCompletion(ref.Child("UpdateChildren").SetValue(sample_values),
                      "UpdateSetValues");

    // Set each key's value to what's given in this map. We use a map of
    // Variant so that we can specify Variant::Null() to remove a key from the
    // database.
    std::map<std::string, firebase::Variant> update_values;
    update_values.insert(std::make_pair("Apple", 100));
    update_values.insert(std::make_pair("Durian", "is a fruit!"));
    update_values.insert(std::make_pair("Eggplant", firebase::Variant::Null()));
    update_values.insert(std::make_pair("Fig", 6));

    WaitForCompletion(ref.Child("UpdateChildren").UpdateChildren(update_values),
                      "UpdateChildren");

    // Get the values that were written to ensure they were updated properly.
    firebase::Future<firebase::database::DataSnapshot> updated_values =
        ref.Child("UpdateChildren").GetValue();
    WaitForCompletion(updated_values, "UpdateChildrenResult");
    if (updated_values.error() == firebase::database::kErrorNone) {
      const firebase::database::DataSnapshot& result = *updated_values.result();
      bool failed = false;
      if (result.children_count() != 5) {
        LogMessage(
            "ERROR: UpdateChildren returned an unexpected number of "
            "children: "
            "%d",
            result.children_count());
        failed = true;
      }
      if (!result.HasChild("Apple") ||
          result.Child("Apple").value().AsInt64() != 100) {
        LogMessage("ERROR: Child key 'Apple' was not updated correctly.");
        failed = true;
      }
      if (!result.HasChild("Banana") ||
          result.Child("Banana").value().AsInt64() != 2) {
        LogMessage("ERROR: Child key 'Banana' was not updated correctly.");
        failed = true;
      }
      if (!result.HasChild("Cranberry") ||
          result.Child("Cranberry").value().AsInt64() != 3) {
        LogMessage("ERROR: Child key 'Cranberry' was not updated correctly.");
        failed = true;
      }
      if (!result.HasChild("Durian") ||
          result.Child("Durian").value().AsString() != "is a fruit!") {
        LogMessage("ERROR: Child key 'Durian' was not updated correctly.");
        failed = true;
      }
      if (result.HasChild("Eggplant")) {
        LogMessage("ERROR: Child key 'Eggplant' was not removed.");
        failed = true;
      }
      if (!result.HasChild("Fig") ||
          result.Child("Fig").value().AsInt64() != 6) {
        LogMessage("ERROR: Child key 'Fig' was not added correctly.");
        failed = true;
      }
      if (!failed) {
        LogMessage("SUCCESS: UpdateChildren succeeded.");
      } else {
        LogMessage(
            "ERROR: UpdateChildren did not modify the children as expected.");
      }
    } else {
      LogMessage("ERROR: Couldn't retrieve updated values.");
    }
  }

  // Test Query, which gives you different views into the same location in the
  // database.
  {
    LogMessage("TEST: Query filtering.");

    firebase::Future<void> set_future =
        ref.Child("QueryFiltering").SetValue(sample_values);
    WaitForCompletion(set_future, "QuerySetValues");
    // Create a query for keys in the lexicographical range "B" to "Dz".
    auto b_to_d = ref.Child("QueryFiltering")
                      .OrderByKey()
                      .StartAt("B")
                      .EndAt("Dz")
                      .GetValue();
    // Create a query for values in the numeric range 1 to 3.
    auto one_to_three = ref.Child("QueryFiltering")
                            .OrderByValue()
                            .StartAt(1)
                            .EndAt(3)
                            .GetValue();
    // Create a query ordered by value, but limited to only the highest two
    // values.
    auto four_and_five =
        ref.Child("QueryFiltering").OrderByValue().LimitToLast(2).GetValue();
    // Create a query ordered by key, but limited to only the lowest two keys.
    auto a_and_b =
        ref.Child("QueryFiltering").OrderByKey().LimitToFirst(2).GetValue();
    // Create a query limited only to the key "Cranberry".
    auto c_only = ref.Child("QueryFiltering")
                      .OrderByKey()
                      .EqualTo("Cranberry")
                      .GetValue();

    WaitForCompletion(b_to_d, "QueryBthruD");
    WaitForCompletion(one_to_three, "Query1to3");
    WaitForCompletion(four_and_five, "Query4and5");
    WaitForCompletion(a_and_b, "QueryAandB");
    WaitForCompletion(c_only, "QueryC");

    bool failed = false;
    // Check that the queries each returned the expected results.
    if (b_to_d.error() != firebase::database::kErrorNone ||
        b_to_d.result()->children_count() != 3 ||
        !b_to_d.result()->HasChild("Banana") ||
        !b_to_d.result()->HasChild("Cranberry") ||
        !b_to_d.result()->HasChild("Durian")) {
      LogMessage("ERROR: Query B-to-D returned unexpected results.");
      failed = true;
    }
    if (one_to_three.error() != firebase::database::kErrorNone ||
        one_to_three.result()->children_count() != 3 ||
        !one_to_three.result()->HasChild("Apple") ||
        !one_to_three.result()->HasChild("Banana") ||
        !one_to_three.result()->HasChild("Cranberry")) {
      LogMessage("ERROR: Query 1-to-3 returned unexpected results.");
      failed = true;
    }
    if (four_and_five.error() != firebase::database::kErrorNone ||
        four_and_five.result()->children_count() != 2 ||
        !four_and_five.result()->HasChild("Durian") ||
        !four_and_five.result()->HasChild("Eggplant")) {
      LogMessage("ERROR: Query 4-and-5 returned unexpected results.");
      failed = true;
    }
    if (a_and_b.error() != firebase::database::kErrorNone ||
        a_and_b.result()->children_count() != 2 ||
        !a_and_b.result()->HasChild("Apple") ||
        !a_and_b.result()->HasChild("Banana")) {
      LogMessage("ERROR: Query A-and-B returned unexpected results.");
      failed = true;
    }
    if (c_only.error() != firebase::database::kErrorNone ||
        c_only.result()->children_count() != 1 ||
        !c_only.result()->HasChild("Cranberry")) {
      LogMessage("ERROR: Query C-only returned unexpected results.");
      failed = true;
    }
    if (!failed) {
      LogMessage("SUCCESS: Query filtering succeeded.");
    }
  }

  // Test a ValueListener, which sits on a Query and listens for changes in
  // the value at that location.
  {
    LogMessage("TEST: ValueListener");
    SampleValueListener* listener = new SampleValueListener();
    WaitForCompletion(ref.Child("ValueListener").SetValue(0), "SetValueZero");
    // Attach the listener, then set 3 values, which will trigger the
    // listener.
    ref.Child("ValueListener").AddValueListener(listener);

    // The listener's OnChanged callback is triggered once when the listener is
    // attached and again every time the data, including children, changes.
    // Wait for here for a moment for the initial values to be received.
    ProcessEvents(2000);

    WaitForCompletion(ref.Child("ValueListener").SetValue(1), "SetValueOne");
    WaitForCompletion(ref.Child("ValueListener").SetValue(2), "SetValueTwo");
    WaitForCompletion(ref.Child("ValueListener").SetValue(3), "SetValueThree");

    LogMessage("  Waiting for ValueListener...");

    // Wait a few seconds for the value listener to be triggered.
    ProcessEvents(2000);

    // Unregister the listener, so it stops triggering.
    ref.Child("ValueListener").RemoveValueListener(listener);

    // Ensure that the listener is not triggered once removed.
    WaitForCompletion(ref.Child("ValueListener").SetValue(4), "SetValueFour");

    // Wait a few more seconds to ensure the listener is not triggered.
    ProcessEvents(2000);

    // Ensure that the listener was only triggered 4 times, with the values
    // 0 (the initial value), 1, 2, and 3.
    if (listener->num_seen_values() == 4 && listener->seen_value(0) &&
        listener->seen_value(1) && listener->seen_value(2) &&
        listener->seen_value(3)) {
      LogMessage("SUCCESS: ValueListener got all values.");
    } else {
      LogMessage("ERROR: ValueListener did not get all values.");
    }

    delete listener;
  }
  // Test a ChildListener, which sits on a Query and listens for changes in
  // the child hierarchy at the location.
  {
    LogMessage("TEST: ChildListener");
    SampleChildListener* listener = new SampleChildListener();

    // Set a child listener that only listens for entities of type "enemy".
    auto entity_list = ref.Child("ChildListener").Child("entity_list");

    entity_list.OrderByChild("entity_type")
        .EqualTo("enemy")
        .AddChildListener(listener);

    // The listener's OnChanged callback is triggered once when the listener is
    // attached and again every time the data, including children, changes.
    // Wait for here for a moment for the initial values to be received.
    ProcessEvents(2000);

    std::map<std::string, std::string> params;
    params["entity_name"] = "cobra";
    params["entity_type"] = "enemy";
    WaitForCompletion(entity_list.Child("0").SetValueAndPriority(params, 0),
                      "SetEntity0");
    params["entity_name"] = "warrior";
    params["entity_type"] = "hero";
    WaitForCompletion(entity_list.Child("1").SetValueAndPriority(params, 10),
                      "SetEntity1");
    params["entity_name"] = "wizard";
    params["entity_type"] = "hero";
    WaitForCompletion(entity_list.Child("2").SetValueAndPriority(params, 20),
                      "SetEntity2");
    params["entity_name"] = "rat";
    params["entity_type"] = "enemy";
    WaitForCompletion(entity_list.Child("3").SetValueAndPriority(params, 30),
                      "SetEntity3");
    params["entity_name"] = "thief";
    params["entity_type"] = "enemy";
    WaitForCompletion(entity_list.Child("4").SetValueAndPriority(params, 40),
                      "SetEntity4");
    params["entity_name"] = "paladin";
    params["entity_type"] = "hero";
    WaitForCompletion(entity_list.Child("5").SetValueAndPriority(params, 50),
                      "SetEntity5");
    params["entity_name"] = "ghost";
    params["entity_type"] = "enemy";
    WaitForCompletion(entity_list.Child("6").SetValueAndPriority(params, 60),
                      "SetEntity6");
    params["entity_name"] = "dragon";
    params["entity_type"] = "enemy";
    WaitForCompletion(entity_list.Child("7").SetValueAndPriority(params, 70),
                      "SetEntity7");
    // Now the thief becomes a hero!
    WaitForCompletion(
        entity_list.Child("4").Child("entity_type").SetValue("hero"),
        "SetEntity4Type");
    // Now the dragon becomes a super-dragon!
    WaitForCompletion(
        entity_list.Child("7").Child("entity_name").SetValue("super-dragon"),
        "SetEntity7Name");
    // Now the super-dragon becomes an mega-dragon!
    WaitForCompletion(
        entity_list.Child("7").Child("entity_name").SetValue("mega-dragon"),
        "SetEntity7NameAgain");
    // And now we change a hero entity, which the Query ignores.
    WaitForCompletion(
        entity_list.Child("2").Child("entity_name").SetValue("super-wizard"),
        "SetEntity2Value");
    // Now poof, the mega-dragon is gone.
    WaitForCompletion(entity_list.Child("7").RemoveValue(), "RemoveEntity7");

    LogMessage("  Waiting for ChildListener...");

    // Wait a few seconds for the child listener to be triggered.
    ProcessEvents(2000);

    // Unregister the listener, so it stops triggering.
    entity_list.OrderByChild("entity_type")
        .EqualTo("enemy")
        .RemoveChildListener(listener);

    // Wait a few seconds for the child listener to be triggered.
    ProcessEvents(2000);

    // Make one more change, to ensure the listener has been removed.
    WaitForCompletion(entity_list.Child("6").SetPriority(0),
                      "SetEntity6Priority");

    // We are expecting to have the following events:
    bool failed = false;
    if (listener->num_events("added 0") != 1) {
      LogMessage(
          "ERROR: OnChildAdded(0) was called an incorrect number of times.");
      failed = true;
    }
    if (listener->num_events("added 3") != 1) {
      LogMessage(
          "ERROR: OnChildAdded(3) was called an incorrect number of times.");
      failed = true;
    }
    if (listener->num_events("added 4") != 1) {
      LogMessage(
          "ERROR: OnChildAdded(4) was called an incorrect number of times.");
      failed = true;
    }
    if (listener->num_events("added 6") != 1) {
      LogMessage(
          "ERROR: OnChildAdded(6) was called an incorrect number of times.");
      failed = true;
    }
    if (listener->num_events("added 7") != 1) {
      LogMessage(
          "ERROR: OnChildAdded(7) was called an incorrect number of times.");
      failed = true;
    }
    if (listener->num_events("removed 4") != 1) {
      LogMessage(
          "ERROR: OnChildRemoved(4) was called an incorrect number of "
          "times.");
      failed = true;
    }
    if (listener->num_events("changed 7") != 2) {
      LogMessage(
          "ERROR: OnChildChanged(7) was called an incorrect number of "
          "times.");
      failed = true;
    }
    if (listener->num_events("removed 7") != 1) {
      LogMessage(
          "ERROR: OnChildRemoved(7) was called an incorrect number of "
          "times.");
      failed = true;
    }
    if (listener->total_events() != 9) {
      LogMessage("ERROR: ChildListener got an incorrect number of events.");
      failed = true;
    }
    if (!failed) {
      LogMessage("SUCCESS: ChildListener got all child events.");
    }
    delete listener;
  }

  // Now check OnDisconnect. When you set an OnDisconnect handler for a
  // database location, an operation will be performed on that location when
  // you disconnect from Firebase Database. In this sample app, we replicate
  // this by shutting down Firebase Database, then starting it up again and
  // checking to see if the OnDisconnect actions were performed.
  {
    LogMessage("TEST: OnDisconnect");
    WaitForCompletion(ref.Child("OnDisconnectTests")
                          .Child("SetValueTo1")
                          .OnDisconnect()
                          ->SetValue(1),
                      "OnDisconnectSetValue1");
    WaitForCompletion(ref.Child("OnDisconnectTests")
                          .Child("SetValue2Priority3")
                          .OnDisconnect()
                          ->SetValueAndPriority(2, 3),
                      "OnDisconnectSetValue2Priority3");
    WaitForCompletion(ref.Child("OnDisconnectTests")
                          .Child("SetValueButThenCancel")
                          .OnDisconnect()
                          ->SetValue("Going to cancel this"),
                      "OnDisconnectSetValueToCancel");
    WaitForCompletion(ref.Child("OnDisconnectTests")
                          .Child("SetValueButThenCancel")
                          .OnDisconnect()
                          ->Cancel(),
                      "OnDisconnectCancel");
    // Set a value that we will then remove on disconnect.
    WaitForCompletion(ref.Child("OnDisconnectTests")
                          .Child("RemoveValue")
                          .SetValue("Will be removed"),
                      "SetValueToRemove");
    WaitForCompletion(ref.Child("OnDisconnectTests")
                          .Child("RemoveValue")
                          .OnDisconnect()
                          ->RemoveValue(),
                      "OnDisconnectRemoveValue");
    // Set up a map to pass to OnDisconnect()->UpdateChildren().
    std::map<std::string, int> children;
    children.insert(std::make_pair("one", 1));
    children.insert(std::make_pair("two", 2));
    children.insert(std::make_pair("three", 3));
    WaitForCompletion(ref.Child("OnDisconnectTests")
                          .Child("UpdateChildren")
                          .OnDisconnect()
                          ->UpdateChildren(children),
                      "OnDisconnectUpdateChildren");
    LogMessage("  Disconnection handlers registered.");
  }

  // Go offline, wait a moment, then go online again. We set up a
  // ValueListener
  // on one of the OnDisconnect locations we set above, so we can see when the
  // disconnection triggers.
  {
    ExpectValueListener* listener = new ExpectValueListener(1);
    ref.Child("OnDisconnectTests")
        .Child("SetValueTo1")
        .AddValueListener(listener);

    LogMessage("  Disconnecting from Firebase Database.");
    database->GoOffline();

    while (!listener->got_value()) {
      ProcessEvents(100);
    }
    ref.Child("OnDisconnectTests")
        .Child("SetValueTo1")
        .RemoveValueListener(listener);
    delete listener;
    listener = nullptr;

    LogMessage("  Reconnecting to Firebase Database.");
    database->GoOnline();
  }

  /// Check that the DisconnectionHandler actions were performed.
  /// Get a brand new reference to the location to be sure.
  ref = database->GetReferenceFromUrl(saved_url.c_str());

  firebase::Future<firebase::database::DataSnapshot> future =
      ref.Child("OnDisconnectTests").GetValue();
  WaitForCompletion(future, "ReadOnDisconnectChanges");
  bool failed = false;

  if (future.error() == firebase::database::kErrorNone) {
    const firebase::database::DataSnapshot& result = *future.result();
    if (!result.HasChild("SetValueTo1") ||
        result.Child("SetValueTo1").value().AsInt64().int64_value() != 1) {
      LogMessage("ERROR: OnDisconnect.SetValue(1) failed.");
      failed = true;
    }
    if (!result.HasChild("SetValue2Priority3") ||
        result.Child("SetValue2Priority3").value().AsInt64().int64_value() !=
            2 ||
        result.Child("SetValue2Priority3").priority().AsInt64().int64_value() !=
            3) {
      LogMessage("ERROR: OnDisconnect.SetValueAndPriority(2, 3) failed.");
      failed = true;
    }
    if (result.HasChild("RemoveValue")) {
      LogMessage("ERROR: OnDisconnect.RemoveValue() failed.");
      failed = true;
    }
    if (result.HasChild("SetValueButThenCancel")) {
      LogMessage("ERROR: OnDisconnect.Cancel() failed.");
      failed = true;
    }
    if (!result.HasChild("UpdateChildren") ||
        !result.Child("UpdateChildren").HasChild("one") ||
        result.Child("UpdateChildren")
                .Child("one")
                .value()
                .AsInt64()
                .int64_value() != 1 ||
        !result.Child("UpdateChildren").HasChild("two") ||
        result.Child("UpdateChildren")
                .Child("two")
                .value()
                .AsInt64()
                .int64_value() != 2 ||
        !result.Child("UpdateChildren").HasChild("three") ||
        result.Child("UpdateChildren")
                .Child("three")
                .value()
                .AsInt64()
                .int64_value() != 3) {
      LogMessage("ERROR: OnDisconnect.UpdateChildren() failed.");
      failed = true;
    }

    if (!failed) {
      LogMessage("SUCCESS: OnDisconnect values were written properly.");
    }
  } else {
    LogMessage("ERROR: Couldn't read OnDisconnect changes, error %d: %s.",
               future.error(), future.error_message());
  }

  bool test_snapshot_was_valid = false;
  firebase::database::DataSnapshot* test_snapshot = nullptr;
  if (future.error() == firebase::database::kErrorNone) {
    // This is a little convoluted as it's not possible to construct an
    // empty test snapshot so we copy the result and point at the copy.
    static firebase::database::DataSnapshot copied_snapshot =  // NOLINT
        *future.result();                                      // NOLINT
    test_snapshot = &copied_snapshot;
    test_snapshot_was_valid = test_snapshot->is_valid();
  }

  LogMessage("Shutdown the Database library.");
  delete database;
  database = nullptr;

  // Ensure that the ref we had is now invalid.
  if (!ref.is_valid()) {
    LogMessage("SUCCESS: Reference was invalidated on library shutdown.");
  } else {
    LogMessage("ERROR: Reference is still valid after library shutdown.");
  }

  if (test_snapshot_was_valid && test_snapshot) {
    if (!test_snapshot->is_valid()) {
      LogMessage("SUCCESS: Snapshot was invalidated on library shutdown.");
    } else {
      LogMessage("ERROR: Snapshot is still valid after library shutdown.");
    }
  } else {
    LogMessage(
        "WARNING: Snapshot was already invalid at shutdown, couldn't check.");
  }

  LogMessage("Signing out from anonymous account.");
  auth->SignOut();
  LogMessage("Shutdown the Auth library.");
  delete auth;
  auth = nullptr;

  LogMessage("Shutdown Firebase App.");
  delete app;

  // Wait until the user wants to quit the app.
  while (!ProcessEvents(1000)) {
  }

  return 0;
}
