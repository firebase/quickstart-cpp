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

#include <ctime>
#include <sstream>
#include <string>

#include "firebase/app.h"
#include "firebase/auth.h"
#include "firebase/util.h"

// Thin OS abstraction layer.
#include "main.h"  // NOLINT

using ::firebase::App;
using ::firebase::AppOptions;
using ::firebase::Future;
using ::firebase::FutureBase;
using ::firebase::auth::Auth;
using ::firebase::auth::Credential;
using ::firebase::auth::AuthError;
using ::firebase::auth::kAuthErrorNone;
using ::firebase::auth::kAuthErrorFailure;
using ::firebase::auth::EmailAuthProvider;
using ::firebase::auth::FacebookAuthProvider;
using ::firebase::auth::GitHubAuthProvider;
using ::firebase::auth::GoogleAuthProvider;
using ::firebase::auth::TwitterAuthProvider;
using ::firebase::auth::User;
using ::firebase::auth::UserInfoInterface;

// Set this to true, and set the email/password, to test a custom email address.
static const bool kTestCustomEmail = false;
static const char kCustomEmail[] = "custom.email@example.com";
static const char kCustomPassword[] = "CustomPasswordGoesHere";

// Constants used during tests.
static const char kTestPassword[] = "testEmailPassword123";
static const char kTestEmailBad[] = "bad.test.email@example.com";
static const char kTestPasswordBad[] = "badTestPassword";
static const char kTestIdTokenBad[] = "bad id token for testing";
static const char kTestAccessTokenBad[] = "bad access token for testing";
static const char kTestPasswordUpdated[] = "testpasswordupdated";

static const char kFirebaseProviderId[] =
#if defined(__ANDROID__)
    "firebase";
#else   // !defined(__ANDROID__)
    "Firebase";
#endif  // !defined(__ANDROID__)

// Don't return until `future` is complete.
// Print a message for whether the result mathes our expectations.
// Returns true if the application should exit.
static bool WaitForFuture(FutureBase future, const char* fn,
                          AuthError expected_error, bool log_error = true) {
  // Note if the future has not be started properly.
  if (future.Status() == ::firebase::kFutureStatusInvalid) {
    LogMessage("ERROR: Future for %s is invalid", fn);
    return false;
  }

  // Wait for future to complete.
  LogMessage("  Calling %s...", fn);
  while (future.Status() == ::firebase::kFutureStatusPending) {
    if (ProcessEvents(100)) return true;
  }

  // Log error result.
  if (log_error) {
    const AuthError error = static_cast<AuthError>(future.Error());
    if (error == expected_error) {
      const char* error_message = future.ErrorMessage();
      if (error_message) {
        LogMessage("%s completed as expected", fn);
      } else {
        LogMessage("%s completed as expected, error: %d '%s'", fn, error,
                   error_message);
      }
    } else {
      LogMessage("ERROR: %s completed with error: %d, `%s`", fn, error,
                 future.ErrorMessage());
    }
  }
  return false;
}

static bool WaitForSignInFuture(Future<User*> sign_in_future, const char* fn,
                                AuthError expected_error, Auth* auth) {
  if (WaitForFuture(sign_in_future, fn, expected_error)) return true;

  const User* const* sign_in_user_ptr = sign_in_future.Result();
  const User* sign_in_user =
      sign_in_user_ptr == nullptr ? nullptr : *sign_in_user_ptr;
  const User* auth_user = auth->CurrentUser();

  if (sign_in_user != auth_user) {
    LogMessage("ERROR: future's user (%x) and CurrentUser (%x) don't match",
               static_cast<int>(reinterpret_cast<intptr_t>(sign_in_user)),
               static_cast<int>(reinterpret_cast<intptr_t>(auth_user)));
  }

  const bool should_be_null = expected_error != kAuthErrorNone;
  const bool is_null = sign_in_user == nullptr;
  if (should_be_null != is_null) {
    LogMessage("ERROR: user pointer (%x) is incorrect",
               static_cast<int>(reinterpret_cast<intptr_t>(auth_user)));
  }
  return false;
}

// Create an email that will be different from previous runs.
// Useful for testing creating new accounts.
static std::string CreateNewEmail() {
  std::stringstream email;
  email << "random_" << std::time(0) << "@gmail.com";
  return email.str();
}

static void ExpectFalse(const char* test, bool value) {
  if (value) {
    LogMessage("ERROR: %s is true instead of false", test);
  } else {
    LogMessage("%s is false, as expected", test);
  }
}

static void ExpectTrue(const char* test, bool value) {
  if (value) {
    LogMessage("%s is true, as expected", test);
  } else {
    LogMessage("ERROR: %s is false instead of true", test);
  }
}

// Log results of a string comparison for `test`.
static void ExpectStringsEqual(const char* test, const char* expected,
                               const char* actual) {
  if (strcmp(expected, actual) == 0) {
    LogMessage("%s is '%s' as expected", test, actual);
  } else {
    LogMessage("ERROR: %s is '%s' instead of '%s'", test, actual, expected);
  }
}

class AuthStateChangeCounter : public firebase::auth::AuthStateListener {
 public:
  AuthStateChangeCounter() : num_state_changes_(0) {}

  virtual void OnAuthStateChanged(Auth* auth) { num_state_changes_++; }

  void CompleteTest(const char* test_name, int expected_state_changes) {
    CompleteTest(test_name, expected_state_changes, expected_state_changes);
  }

  void CompleteTest(const char* test_name, int min_state_changes,
                    int max_state_changes) {
    const bool success = min_state_changes <= num_state_changes_ &&
                         num_state_changes_ <= max_state_changes;
    LogMessage("%sAuthStateListener called %d time%s on %s.",
               success ? "" : "ERROR: ", num_state_changes_,
               num_state_changes_ == 1 ? "" : "s", test_name);
    num_state_changes_ = 0;
  }

 private:
  int num_state_changes_;
};

// Utility class for holding a user's login credentials.
class UserLogin {
 public:
  UserLogin(Auth* auth, const std::string& email, const std::string& password)
      : auth_(auth),
        email_(email),
        password_(password),
        user_(nullptr),
        log_errors_(true) {}

  explicit UserLogin(Auth* auth) : auth_(auth) {
    email_ = CreateNewEmail();
    password_ = kTestPassword;
  }

  ~UserLogin() {
    if (user_ != nullptr) {
      log_errors_ = false;
      Delete();
    }
  }

  void Register() {
    Future<User*> register_test_account =
        auth_->CreateUserWithEmailAndPassword(email(), password());
    WaitForSignInFuture(register_test_account,
                        "CreateUserWithEmailAndPassword() to create temp user",
                        kAuthErrorNone, auth_);
    user_ = register_test_account.Result() ? *register_test_account.Result()
                                           : nullptr;
  }

  void Login() {
    Credential email_cred =
        EmailAuthProvider::GetCredential(email(), password());
    Future<User*> sign_in_cred = auth_->SignInWithCredential(email_cred);
    WaitForSignInFuture(sign_in_cred,
                        "Auth::SignInWithCredential() for UserLogin",
                        kAuthErrorNone, auth_);
  }

  void Delete() {
    if (user_ != nullptr) {
      Future<void> delete_future = user_->Delete();
      if (delete_future.Status() == ::firebase::kFutureStatusInvalid) {
        Login();
        delete_future = user_->Delete();
      }

      WaitForFuture(delete_future, "User::Delete()", kAuthErrorNone,
                    log_errors_);
    }
    user_ = nullptr;
  }

  const char* email() const { return email_.c_str(); }
  const char* password() const { return password_.c_str(); }
  User* user() const { return user_; }
  void set_email(const char* email) { email_ = email; }
  void set_password(const char* password) { password_ = password; }

 private:
  Auth* auth_;
  std::string email_;
  std::string password_;
  User* user_;
  bool log_errors_;
};

// Execute all methods of the C++ Auth API.
extern "C" int common_main(int argc, const char* argv[]) {
  App* app;
  LogMessage("Starting Auth tests.");

#if defined(__ANDROID__)
  app = App::Create(GetJniEnv(), GetActivity());
#else
  app = App::Create();
#endif  // defined(__ANDROID__)

  LogMessage("Created the Firebase app %x.",
             static_cast<int>(reinterpret_cast<intptr_t>(app)));
  // Create the Auth class for that App.

  ::firebase::ModuleInitializer initializer;
  initializer.Initialize(app, nullptr, [](::firebase::App* app, void*) {
    ::firebase::InitResult init_result;
    Auth::GetAuth(app, &init_result);
    return init_result;
  });
  while (initializer.InitializeLastResult().Status() !=
         firebase::kFutureStatusComplete) {
    if (ProcessEvents(100)) return 1;  // exit if requested
  }

  if (initializer.InitializeLastResult().Error() != 0) {
    LogMessage("Failed to initialize Auth: %s",
               initializer.InitializeLastResult().ErrorMessage());
    ProcessEvents(2000);
    return 1;
  }

  Auth* auth = Auth::GetAuth(app);

  LogMessage("Created the Auth %x class for the Firebase app.",
             static_cast<int>(reinterpret_cast<intptr_t>(auth)));

  // It's possible for CurrentUser() to be non-null if the previous run
  // left us in a signed-in state.
  if (auth->CurrentUser() == nullptr) {
    LogMessage("No user signed in at creation time.");
  } else {
    LogMessage("Current user %s already signed in, so signing them out.",
               auth->CurrentUser()->DisplayName().c_str());
    auth->SignOut();
  }

  // --- Credential copy tests -------------------------------------------------
  {
    Credential email_cred = EmailAuthProvider::GetCredential(kCustomEmail,
                                                             kCustomPassword);
    Credential facebook_cred =
        FacebookAuthProvider::GetCredential(kTestAccessTokenBad);

    // Test copy constructor.
    Credential cred_copy(email_cred);

    // Test assignment operator.
    cred_copy = facebook_cred;
    (void)cred_copy;
  }

  // --- Custom Profile tests --------------------------------------------------
  {
    if (kTestCustomEmail) {
      // Test Auth::SignInWithEmailAndPassword().
      // Sign in with email and password that have already been registered.
      Future<User*> sign_in_future =
          auth->SignInWithEmailAndPassword(kCustomEmail, kCustomPassword);
      WaitForSignInFuture(sign_in_future,
                          "Auth::SignInWithEmailAndPassword() existing "
                          "(custom) email and password",
                          kAuthErrorNone, auth);
      // Test SignOut() after signed in with email and password.
      if (sign_in_future.Status() == ::firebase::kFutureStatusComplete) {
        auth->SignOut();
        if (auth->CurrentUser() != nullptr) {
          LogMessage(
              "ERROR: CurrentUser() returning %x instead of NULL after "
              "SignOut()",
              auth->CurrentUser());
        }
      }
    }
  }

  // --- StateChange tests -----------------------------------------------------
  {
    AuthStateChangeCounter counter;

    // Test notification on registration.
    auth->AddAuthStateListener(&counter);
    counter.CompleteTest("registration", 0);

    // Test notification on SignOut(), when already signed-out.
    auth->SignOut();
    counter.CompleteTest("SignOut() when already signed-out", 0);

    // Test notification on SignIn().
    Future<User*> sign_in_future = auth->SignInAnonymously();
    WaitForSignInFuture(sign_in_future, "Auth::SignInAnonymously()",
                        kAuthErrorNone, auth);
    counter.CompleteTest("SignInAnonymously()", 1, 4);

    // Test notification on SignOut(), when signed-in.
    // TODO(jsanmiya): Change the minimum expected callbacks to 1 once
    // b/32179003 is fixed.
    auth->SignOut();
    counter.CompleteTest("SignOut()", 0, 4);

    auth->RemoveAuthStateListener(&counter);
  }

  // --- Auth tests ------------------------------------------------------------
  {
    UserLogin user_login(auth);  // Generate a random name/password
    user_login.Register();
    if (!user_login.user()) {
      LogMessage("ERROR: Could not register new user.");
    } else {
      // Test Auth::SignInAnonymously().
      {
        Future<User*> sign_in_future = auth->SignInAnonymously();
        WaitForSignInFuture(sign_in_future, "Auth::SignInAnonymously()",
                            kAuthErrorNone, auth);
        // Test SignOut() after signed in anonymously.
        if (sign_in_future.Status() == ::firebase::kFutureStatusComplete) {
          auth->SignOut();
          if (auth->CurrentUser() != nullptr) {
            LogMessage(
                "ERROR: CurrentUser() returning %x instead of NULL after "
                "SignOut()",
                auth->CurrentUser());
          }
        }
      }

      // Test Auth::FetchProvidersForEmail().
      {
        Future<Auth::FetchProvidersResult> providers_future =
            auth->FetchProvidersForEmail(user_login.email());
        WaitForFuture(providers_future, "Auth::FetchProvidersForEmail()",
                      kAuthErrorNone);
        const Auth::FetchProvidersResult* pro = providers_future.Result();
        if (pro) {
          LogMessage("  email %s, num providers %d", user_login.email(),
                     pro->providers.size());
          for (auto it = pro->providers.begin(); it != pro->providers.end();
               ++it) {
            LogMessage("    * %s", it->c_str());
          }
        }
      }

      // Test Auth::SignInWithEmailAndPassword().
      // Sign in with email and password that have already been registered.
      {
        Future<User*> sign_in_future = auth->SignInWithEmailAndPassword(
            user_login.email(), user_login.password());
        WaitForSignInFuture(
            sign_in_future,
            "Auth::SignInWithEmailAndPassword() existing email and password",
            kAuthErrorNone, auth);
        // Test SignOut() after signed in with email and password.
        if (sign_in_future.Status() == ::firebase::kFutureStatusComplete) {
          auth->SignOut();
          if (auth->CurrentUser() != nullptr) {
            LogMessage(
                "ERROR: CurrentUser() returning %x instead of NULL after "
                "SignOut()",
                auth->CurrentUser());
          }
        }
      }

      // Sign in user with bad email. Should fail.
      {
        Future<User*> sign_in_future_bad_email =
            auth->SignInWithEmailAndPassword(kTestEmailBad, kTestPassword);
        WaitForSignInFuture(sign_in_future_bad_email,
                            "Auth::SignInWithEmailAndPassword() bad email",
                            kAuthErrorFailure, auth);
      }

      // Sign in user with correct email but bad password. Should fail.
      {
        Future<User*> sign_in_future_bad_password =
            auth->SignInWithEmailAndPassword(user_login.email(),
                                             kTestPasswordBad);
        WaitForSignInFuture(sign_in_future_bad_password,
                            "Auth::SignInWithEmailAndPassword() bad password",
                            kAuthErrorFailure, auth);
      }

      // Try to create with existing email. Should fail.
      {
        Future<User*> create_future_bad = auth->CreateUserWithEmailAndPassword(
            user_login.email(), user_login.password());
        WaitForSignInFuture(
            create_future_bad,
            "Auth::CreateUserWithEmailAndPassword() existing email",
            kAuthErrorFailure, auth);
      }

      // Test Auth::SignInWithCredential() using email&password.
      // Use existing email. Should succeed.
      {
        Credential email_cred_ok = EmailAuthProvider::GetCredential(
            user_login.email(), user_login.password());
        Future<User*> sign_in_cred_ok =
            auth->SignInWithCredential(email_cred_ok);
        WaitForSignInFuture(sign_in_cred_ok,
                            "Auth::SignInWithCredential() existing email",
                            kAuthErrorNone, auth);
      }

      // Use bad Facebook credentials. Should fail.
      {
        Credential facebook_cred_bad =
            FacebookAuthProvider::GetCredential(kTestAccessTokenBad);
        Future<User*> facebook_bad =
            auth->SignInWithCredential(facebook_cred_bad);
        WaitForSignInFuture(
            facebook_bad,
            "Auth::SignInWithCredential() bad Facebook credentials",
            kAuthErrorFailure, auth);
      }

      // Use bad GitHub credentials. Should fail.
      {
        Credential git_hub_cred_bad =
            GitHubAuthProvider::GetCredential(kTestAccessTokenBad);
        Future<User*> git_hub_bad =
            auth->SignInWithCredential(git_hub_cred_bad);
        WaitForSignInFuture(
            git_hub_bad, "Auth::SignInWithCredential() bad GitHub credentials",
            kAuthErrorFailure, auth);
      }

      // Use bad Google credentials. Should fail.
      {
        Credential google_cred_bad = GoogleAuthProvider::GetCredential(
            kTestIdTokenBad, kTestAccessTokenBad);
        Future<User*> google_bad = auth->SignInWithCredential(google_cred_bad);
        WaitForSignInFuture(
            google_bad, "Auth::SignInWithCredential() bad Google credentials",
            kAuthErrorFailure, auth);
      }

      // Use bad Twitter credentials. Should fail.
      {
        Credential twitter_cred_bad = TwitterAuthProvider::GetCredential(
            kTestIdTokenBad, kTestAccessTokenBad);
        Future<User*> twitter_bad =
            auth->SignInWithCredential(twitter_cred_bad);
        WaitForSignInFuture(
            twitter_bad, "Auth::SignInWithCredential() bad Twitter credentials",
            kAuthErrorFailure, auth);
      }

      // Test Auth::SendPasswordResetEmail().
      // Use existing email. Should succeed.
      {
        Future<void> send_password_reset_ok =
            auth->SendPasswordResetEmail(user_login.email());
        WaitForFuture(send_password_reset_ok,
                      "Auth::SendPasswordResetEmail() existing email",
                      kAuthErrorNone);
      }

      // Use bad email. Should fail.
      {
        Future<void> send_password_reset_bad =
            auth->SendPasswordResetEmail(kTestEmailBad);
        WaitForFuture(send_password_reset_bad,
                      "Auth::SendPasswordResetEmail() bad email",
                      kAuthErrorFailure);
      }
    }
  }
  // --- User tests ------------------------------------------------------------
  // Test anonymous user info strings.
  {
    Future<User*> anon_sign_in_for_user = auth->SignInAnonymously();
    WaitForSignInFuture(anon_sign_in_for_user,
                        "Auth::SignInAnonymously() for User", kAuthErrorNone,
                        auth);
    if (anon_sign_in_for_user.Status() == ::firebase::kFutureStatusComplete) {
      User* anonymous_user = anon_sign_in_for_user.Result()
                                 ? *anon_sign_in_for_user.Result()
                                 : nullptr;
      if (anonymous_user != nullptr) {
        LogMessage("Anonymous UID is %s", anonymous_user->UID().c_str());
        ExpectStringsEqual("Anonymous user Email", "",
                           anonymous_user->Email().c_str());
        ExpectStringsEqual("Anonymous user DisplayName", "",
                           anonymous_user->DisplayName().c_str());
        ExpectStringsEqual("Anonymous user PhotoUrl", "",
                           anonymous_user->PhotoUrl().c_str());
        ExpectStringsEqual("Anonymous user ProviderId", kFirebaseProviderId,
                           anonymous_user->ProviderId().c_str());
        ExpectTrue("Anonymous email Anonymous()", anonymous_user->Anonymous());
        ExpectFalse("Email email EmailVerified()",
                    anonymous_user->EmailVerified());

        // Test User::LinkWithCredential().
        const std::string newer_email = CreateNewEmail();
        Credential user_cred = EmailAuthProvider::GetCredential(
            newer_email.c_str(), kTestPassword);
        Future<User*> link_future =
            anonymous_user->LinkWithCredential(user_cred);
        WaitForSignInFuture(link_future, "User::LinkWithCredential()",
                            kAuthErrorNone, auth);

        UserLogin user_login(auth);
        user_login.Register();

        if (!user_login.user()) {
          LogMessage("Error - Could not create new user.");
        } else {
          // Test email user info strings.
          Future<User*> email_sign_in_for_user =
              auth->SignInWithEmailAndPassword(user_login.email(),
                                               user_login.password());
          WaitForSignInFuture(email_sign_in_for_user,
                              "Auth::SignInWithEmailAndPassword() for User",
                              kAuthErrorNone, auth);
          User* email_user = email_sign_in_for_user.Result()
                                 ? *email_sign_in_for_user.Result()
                                 : nullptr;
          if (email_user != nullptr) {
            LogMessage("Email UID is %s", email_user->UID().c_str());
            ExpectStringsEqual("Email user Email", user_login.email(),
                               email_user->Email().c_str());
            ExpectStringsEqual("Email user DisplayName", "",
                               email_user->DisplayName().c_str());
            ExpectStringsEqual("Email user PhotoUrl", "",
                               email_user->PhotoUrl().c_str());
            ExpectStringsEqual("Email user ProviderId", kFirebaseProviderId,
                               email_user->ProviderId().c_str());
            ExpectFalse("Email email Anonymous()", email_user->Anonymous());
            ExpectFalse("Email email EmailVerified()",
                        email_user->EmailVerified());

            // Test User::Token().
            // with force_refresh = false.
            Future<std::string> token_no_refresh = email_user->Token(false);
            WaitForFuture(token_no_refresh, "User::Token(false)",
                          kAuthErrorNone);
            LogMessage("User::Token(false) = %s",
                       token_no_refresh.Result()
                           ? token_no_refresh.Result()->c_str()
                           : "");

            // with force_refresh = true.
            Future<std::string> token_force_refresh = email_user->Token(true);
            WaitForFuture(token_force_refresh, "User::Token(true)",
                          kAuthErrorNone);
            LogMessage("User::Token(true) = %s",
                       token_force_refresh.Result()
                           ? token_force_refresh.Result()->c_str()
                           : "");

            // Test Reload().
            Future<void> reload_future = email_user->Reload();
            WaitForFuture(reload_future, "User::Reload()", kAuthErrorNone);

            // Test User::RefreshToken().
            const std::string refresh_token = email_user->RefreshToken();
            LogMessage("User::RefreshToken() = %s", refresh_token.c_str());

            // Test User::Unlink().
            Future<User*> unlink_future = email_user->Unlink("firebase");
            WaitForSignInFuture(unlink_future, "User::Unlink()",
                                kAuthErrorFailure, auth);

            // Sign in again if user is now invalid.
            if (auth->CurrentUser() == nullptr) {
              Future<User*> email_sign_in_again =
                  auth->SignInWithEmailAndPassword(user_login.email(),
                                                   user_login.password());
              WaitForSignInFuture(email_sign_in_again,
                                  "Auth::SignInWithEmailAndPassword() again",
                                  kAuthErrorNone, auth);
              email_user = email_sign_in_again.Result()
                               ? *email_sign_in_again.Result()
                               : nullptr;
            }
          }
          if (email_user != nullptr) {
            // Test User::ProviderData().
            const std::vector<UserInfoInterface*>& provider_data =
                email_user->ProviderData();
            LogMessage("User::ProviderData() returned %d interface%s",
                       provider_data.size(),
                       provider_data.size() == 1 ? "" : "s");
            for (size_t i = 0; i < provider_data.size(); ++i) {
              const UserInfoInterface* user_info = provider_data[i];
              LogMessage(
                  "    UID() = %s\n"
                  "    Email() = %s\n"
                  "    DisplayName() = %s\n"
                  "    PhotoUrl() = %s\n"
                  "    ProviderId() = %s",
                  user_info->UID().c_str(), user_info->Email().c_str(),
                  user_info->DisplayName().c_str(),
                  user_info->PhotoUrl().c_str(),
                  user_info->ProviderId().c_str());
            }

            // Test User::UpdateEmail().
            const std::string newest_email = CreateNewEmail();
            Future<void> update_email_future =
                email_user->UpdateEmail(newest_email.c_str());
            WaitForFuture(update_email_future, "User::UpdateEmail()",
                          kAuthErrorNone);

            // Test User::UpdatePassword().
            Future<void> update_password_future =
                email_user->UpdatePassword(kTestPasswordUpdated);
            WaitForFuture(update_password_future, "User::UpdatePassword()",
                          kAuthErrorNone);

            // Test User::Reauthenticate().
            Credential email_cred_reauth = EmailAuthProvider::GetCredential(
                newest_email.c_str(), kTestPasswordUpdated);
            Future<void> reauthenticate_future =
                email_user->Reauthenticate(email_cred_reauth);
            WaitForFuture(reauthenticate_future, "User::Reauthenticate()",
                          kAuthErrorNone);

            // Test User::SendEmailVerification().
            Future<void> send_email_verification_future =
                email_user->SendEmailVerification();
            WaitForFuture(send_email_verification_future,
                          "User::SendEmailVerification()", kAuthErrorNone);
          }
        }
      }
    }

    // Test User::Delete().
    const std::string new_email_for_delete = CreateNewEmail();
    Future<User*> create_future_for_delete =
        auth->CreateUserWithEmailAndPassword(new_email_for_delete.c_str(),
                                             kTestPassword);
    WaitForSignInFuture(
        create_future_for_delete,
        "Auth::CreateUserWithEmailAndPassword() new email for delete",
        kAuthErrorNone, auth);
    User* email_user_for_delete = create_future_for_delete.Result()
                                      ? *create_future_for_delete.Result()
                                      : nullptr;
    if (email_user_for_delete != nullptr) {
      Future<void> delete_future = email_user_for_delete->Delete();
      WaitForFuture(delete_future, "User::Delete()", kAuthErrorNone);
    }
  }
  {
    // We end with a login so that we can test if a second run will detect
    // that we're already logged-in.
    Future<User*> sign_in_future = auth->SignInAnonymously();
    WaitForSignInFuture(sign_in_future, "Auth::SignInAnonymously() at end",
                        kAuthErrorNone, auth);
  }

  LogMessage("Completed Auth tests.");

  while (!ProcessEvents(1000)) {
  }

  delete auth;
  delete app;

  return 0;
}
