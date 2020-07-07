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
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <string>
#include <utility>

#include "firebase/auth.h"
#include "firebase/auth/user.h"
#include "firebase/firestore.h"
#include "firebase/util.h"

// Thin OS abstraction layer.
#include "main.h"  // NOLINT

const int kTimeoutMs = 5000;
const int kSleepMs = 100;

// Waits for a Future to be completed and returns whether the future has
// completed successfully. If the Future returns an error, it will be logged.
bool Await(const firebase::FutureBase& future, const char* name) {
  int remaining_timeout = kTimeoutMs;
  while (future.status() == firebase::kFutureStatusPending &&
         remaining_timeout > 0) {
    remaining_timeout -= kSleepMs;
    ProcessEvents(kSleepMs);
  }

  if (future.status() != firebase::kFutureStatusComplete) {
    LogMessage("ERROR: %s returned an invalid result.", name);
    return false;
  } else if (future.error() != 0) {
    LogMessage("ERROR: %s returned error %d: %s", name, future.error(),
               future.error_message());
    return false;
  }
  return true;
}

class Countable {
 public:
  int event_count() const { return event_count_; }

 protected:
  int event_count_ = 0;
};

template <typename T>
class TestEventListener : public Countable,
                          public firebase::firestore::EventListener<T> {
 public:
  explicit TestEventListener(std::string name) : name_(std::move(name)) {}

  void OnEvent(const T& value,
               const firebase::firestore::Error error) override {
    event_count_++;
    if (error != firebase::firestore::kOk) {
      LogMessage("ERROR: EventListener %s got %d.", name_.c_str(), error);
    }
  }

  // Hides the STLPort-related quirk that `AddSnapshotListener` has different
  // signatures depending on whether `std::function` is available.
  template <typename U>
  firebase::firestore::ListenerRegistration AttachTo(U* ref) {
#if !defined(STLPORT)
    return ref->AddSnapshotListener(
        [this](const T& result, firebase::firestore::Error error) {
          OnEvent(result, error);
        });
#else
    return ref->AddSnapshotListener(this);
#endif
  }

 private:
  std::string name_;
};

void Await(const Countable& listener, const char* name) {
  int remaining_timeout = kTimeoutMs;
  while (listener.event_count() && remaining_timeout > 0) {
    remaining_timeout -= kSleepMs;
    ProcessEvents(kSleepMs);
  }
  if (remaining_timeout <= 0) {
    LogMessage("ERROR: %s listener timed out.", name);
  }
}

extern "C" int common_main(int argc, const char* argv[]) {
  firebase::App* app;

#if defined(__ANDROID__)
  app = firebase::App::Create(GetJniEnv(), GetActivity());
#else
  app = firebase::App::Create();
#endif  // defined(__ANDROID__)

  LogMessage("Initialized Firebase App.");

  LogMessage("Initializing Firebase Auth...");
  firebase::InitResult result;
  firebase::auth::Auth* auth = firebase::auth::Auth::GetAuth(app, &result);
  if (result != firebase::kInitResultSuccess) {
    LogMessage("Failed to initialize Firebase Auth, error: %d",
               static_cast<int>(result));
    return -1;
  }
  LogMessage("Initialized Firebase Auth.");

  LogMessage("Signing in...");
  // Auth caches the previously signed-in user, which can be annoying when
  // trying to test for sign-in failures.
  auth->SignOut();
  auto login_future = auth->SignInAnonymously();
  Await(login_future, "Auth sign-in");
  auto* login_result = login_future.result();
  if (login_result && *login_result) {
    const firebase::auth::User* user = *login_result;
    LogMessage("Signed in as %s user, uid: %s, email: %s.\n",
               user->is_anonymous() ? "an anonymous" : "a non-anonymous",
               user->uid().c_str(), user->email().c_str());
  } else {
    LogMessage("ERROR: could not sign in");
  }

  // Note: Auth cannot be deleted while any of the futures issued by it are
  // still valid.
  login_future.Release();

  LogMessage("Initialize Firebase Firestore.");

  // Use ModuleInitializer to initialize Database, ensuring no dependencies are
  // missing.
  firebase::firestore::Firestore* firestore = nullptr;
  void* initialize_targets[] = {&firestore};

  const firebase::ModuleInitializer::InitializerFn initializers[] = {
      [](firebase::App* app, void* data) {
        LogMessage("Attempt to initialize Firebase Firestore.");
        void** targets = reinterpret_cast<void**>(data);
        firebase::InitResult result;
        *reinterpret_cast<firebase::firestore::Firestore**>(targets[0]) =
            firebase::firestore::Firestore::GetInstance(app, &result);
        return result;
      }};

  firebase::ModuleInitializer initializer;
  initializer.Initialize(app, initialize_targets, initializers,
                         sizeof(initializers) / sizeof(initializers[0]));

  Await(initializer.InitializeLastResult(), "Initialize");

  if (initializer.InitializeLastResult().error() != 0) {
    LogMessage("Failed to initialize Firebase libraries: %s",
               initializer.InitializeLastResult().error_message());
    return -1;
  }
  LogMessage("Successfully initialized Firebase Firestore.");

  firestore->set_log_level(firebase::kLogLevelDebug);

  if (firestore->app() != app) {
    LogMessage("ERROR: failed to get App the Firestore was created with.");
  }

  firebase::firestore::Settings settings = firestore->settings();
  firestore->set_settings(settings);
  LogMessage("Successfully set Firestore settings.");

  LogMessage("Testing non-wrapping types.");
  const firebase::Timestamp timestamp{1, 2};
  if (timestamp.seconds() != 1 || timestamp.nanoseconds() != 2) {
    LogMessage("ERROR: Timestamp creation failed.");
  }
  const firebase::firestore::SnapshotMetadata metadata{
      /*has_pending_writes*/ false, /*is_from_cache*/ true};
  if (metadata.has_pending_writes() || !metadata.is_from_cache()) {
    LogMessage("ERROR: SnapshotMetadata creation failed.");
  }
  const firebase::firestore::GeoPoint point{1.23, 4.56};
  if (point.latitude() != 1.23 || point.longitude() != 4.56) {
    LogMessage("ERROR: GeoPoint creation failed.");
  }
  LogMessage("Tested non-wrapping types.");

  LogMessage("Testing collections.");
  firebase::firestore::CollectionReference collection =
      firestore->Collection("foo");
  if (collection.id() != "foo") {
    LogMessage("ERROR: failed to get collection id.");
  }
  if (collection.Document("bar").path() != "foo/bar") {
    LogMessage("ERROR: failed to get path of a nested document.");
  }
  LogMessage("Tested collections.");

  LogMessage("Testing documents.");
  firebase::firestore::DocumentReference document =
      firestore->Document("foo/bar");
  if (document.firestore() != firestore) {
    LogMessage("ERROR: failed to get Firestore from document.");
  }

  if (document.path() != "foo/bar") {
    LogMessage("ERROR: failed to get path string from document.");
  }

  LogMessage("Testing Set().");
  Await(document.Set(firebase::firestore::MapFieldValue{
            {"str", firebase::firestore::FieldValue::String("foo")},
            {"int", firebase::firestore::FieldValue::Integer(123)}}),
        "document.Set");

  LogMessage("Testing Update().");
  Await(document.Update(firebase::firestore::MapFieldValue{
            {"int", firebase::firestore::FieldValue::Integer(321)}}),
        "document.Update");

  LogMessage("Testing Get().");
  auto doc_future = document.Get();
  if (Await(doc_future, "document.Get")) {
    const firebase::firestore::DocumentSnapshot* snapshot = doc_future.result();
    if (snapshot == nullptr) {
      LogMessage("ERROR: failed to read document.");
    } else {
      for (const auto& kv : snapshot->GetData()) {
        if (kv.second.type() ==
            firebase::firestore::FieldValue::Type::kString) {
          LogMessage("key is %s, value is %s", kv.first.c_str(),
                     kv.second.string_value().c_str());
        } else if (kv.second.type() ==
                   firebase::firestore::FieldValue::Type::kInteger) {
          LogMessage("key is %s, value is %ld", kv.first.c_str(),
                     kv.second.integer_value());
        } else {
          // Log unexpected type for debugging.
          LogMessage("key is %s, value is neither string nor integer",
                     kv.first.c_str());
        }
      }
    }
  }

  LogMessage("Testing Delete().");
  Await(document.Delete(), "document.Delete");
  LogMessage("Tested document operations.");

  TestEventListener<firebase::firestore::DocumentSnapshot>
      document_event_listener{"for document"};
  firebase::firestore::ListenerRegistration registration =
      document_event_listener.AttachTo(&document);
  Await(document_event_listener, "document.AddSnapshotListener");
  registration.Remove();
  LogMessage("Successfully added and removed document snapshot listener.");

  LogMessage("Testing batch write.");
  firebase::firestore::WriteBatch batch = firestore->batch();
  batch.Set(collection.Document("one"),
            firebase::firestore::MapFieldValue{
                {"str", firebase::firestore::FieldValue::String("foo")}});
  batch.Set(collection.Document("two"),
            firebase::firestore::MapFieldValue{
                {"int", firebase::firestore::FieldValue::Integer(123)}});
  Await(batch.Commit(), "batch.Commit");
  LogMessage("Tested batch write.");

  LogMessage("Testing transaction.");
  Await(
      firestore->RunTransaction(
          [collection](firebase::firestore::Transaction& transaction,
                       std::string&) -> firebase::firestore::Error {
            transaction.Update(
                collection.Document("one"),
                firebase::firestore::MapFieldValue{
                    {"int", firebase::firestore::FieldValue::Integer(123)}});
            transaction.Delete(collection.Document("two"));
            transaction.Set(
                collection.Document("three"),
                firebase::firestore::MapFieldValue{
                    {"int", firebase::firestore::FieldValue::Integer(321)}});
            return firebase::firestore::kOk;
          }),
      "firestore.RunTransaction");
  LogMessage("Tested transaction.");

  LogMessage("Testing query.");
  firebase::firestore::Query query =
      collection
          .WhereGreaterThan("int",
                            firebase::firestore::FieldValue::Boolean(true))
          .Limit(3);
  auto query_future = query.Get();
  if (Await(query_future, "query.Get")) {
    const firebase::firestore::QuerySnapshot* snapshot = query_future.result();
    if (snapshot == nullptr) {
      LogMessage("ERROR: failed to fetch query result.");
    } else {
      for (const auto& doc : snapshot->documents()) {
        if (doc.id() == "one" || doc.id() == "three") {
          LogMessage("doc %s is %ld", doc.id().c_str(),
                     doc.Get("int").integer_value());
        } else {
          LogMessage("ERROR: unexpected document %s.", doc.id().c_str());
        }
      }
    }
  } else {
    LogMessage("ERROR: failed to fetch query result.");
  }
  LogMessage("Tested query.");

  LogMessage("Shutdown the Firestore library.");
  delete firestore;
  firestore = nullptr;

  LogMessage("Shutdown Auth.");
  delete auth;
  LogMessage("Shutdown Firebase App.");
  delete app;

  // Log this as the last line to ensure all test cases above goes through.
  // The test harness will check this line appears.
  LogMessage("Tests PASS.");

  // Wait until the user wants to quit the app.
  while (!ProcessEvents(1000)) {
  }

  return 0;
}
