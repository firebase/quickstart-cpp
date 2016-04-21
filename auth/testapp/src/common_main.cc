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

#include <unistd.h>
#include <ctime>
#include <sstream>
#include <string>

#include "firebase/app.h"
#include "firebase/auth.h"

// Thin OS abstraction layer.
#include "main.h"  // NOLINT

using ::firebase::App;
using ::firebase::AppOptions;
using ::firebase::Auth;
using ::firebase::AuthCredential;
using ::firebase::AuthError;
using ::firebase::kAuthError_None;
using ::firebase::AuthErrorText;
using ::firebase::EmailAuthProvider;
using ::firebase::FacebookAuthProvider;
using ::firebase::Future;
using ::firebase::FutureBase;
using ::firebase::GoogleAuthProvider;
using ::firebase::User;
using ::firebase::UserInfoInterface;

// The gmail password for this email is in Valentine.
// The password below is used for Firebear testing only.
static const char kTestEmail[] = "fi.bear.test@gmail.com";
static const char kTestPassword[] = "testpassword";
static const char kTestEmailBad[] = "fi.bear.test.bad@gmail.com";
static const char kTestPasswordBad[] = "testpasswordbad";
static const char kTestCodeBad[] = "bad action code for testing";
static const char kTestIdTokenBad[] = "bad id token for testing";
static const char kTestAccessTokenBad[] = "bad access token for testing";
static const char kTestPasswordUpdated[] = "testpasswordupdated";
static const char kTestDisplayName[] = "my display name";
static const char kTestPhotoUri[] = "my photo uri";

// TODO(jsanmiya): These should be the same between platforms.
// Remove when standardized.
static const char kFirebaseProviderId[] =
#if defined(__ANDROID__)
    "firebase";
#else   // !defined(__ANDROID__)
    "Firebase";
#endif  // !defined(__ANDROID__)

App* g_app;

// Pause the current thread for the specified number of milliseconds.
static void Wait(int ms) {
#if defined(__ANDROID__)
  ProcessEvents(ms);
#else   // !defined(__ANDROID__)
  usleep(1000 * ms);
#endif  // !defined(__ANDROID__)
}

// Don't return until `future` is complete.
// Print a message for whether the result mathes our expectations.
static void WaitForFuture(const FutureBase& future, const char* fn,
                          AuthError expected_error) {
  // Note if the future has not be started properly.
  if (future.Status() == ::firebase::kFutureStatus_Invalid) {
    LogMessage("ERROR! Future for %s is invalid", fn);
  }

  // Wait for future to complete.
  while (future.Status() == ::firebase::kFutureStatus_Pending) {
    Wait(100);
    LogMessage("  Calling %s...", fn);
  }

  // Log error result.
  const AuthError error = static_cast<AuthError>(future.Error());
  if (error == expected_error) {
    LogMessage("%s completed as expected, error `%s`", fn,
               AuthErrorText(error));
  } else {
    LogMessage("ERROR! %s completed with error `%s` (%d) instead of `%s`", fn,
               AuthErrorText(error), error, AuthErrorText(expected_error));
  }
}

static void WaitForSignInFuture(const FutureBase& future, const char* fn,
                                AuthError expected_error, Auth* auth) {
  WaitForFuture(future, fn, expected_error);

  const Future<User*>& sign_in_future =
      static_cast<const Future<User*>&>(future);
  const User* const* sign_in_user_ptr = sign_in_future.Result();
  const User* sign_in_user =
      sign_in_user_ptr == NULL ? NULL : *sign_in_user_ptr;
  const User* auth_user = auth->CurrentUser();

  if (sign_in_user != auth_user) {
    LogMessage("ERROR: future's user (%x) and CurrentUser (%x) don't match",
               static_cast<int>(reinterpret_cast<intptr_t>(sign_in_user)),
               static_cast<int>(reinterpret_cast<intptr_t>(auth_user)));
  }

  const bool should_be_null = expected_error != kAuthError_None;
  const bool is_null = sign_in_user == NULL;
  if (should_be_null != is_null) {
    LogMessage("ERROR: user pointer (%x) is incorrect",
               static_cast<int>(reinterpret_cast<intptr_t>(auth_user)));
  }
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
    LogMessage("ERROR! %s is true instead of false", test);
  } else {
    LogMessage("%s is false, as expected", test);
  }
}

static void ExpectTrue(const char* test, bool value) {
  if (value) {
    LogMessage("%s is true, as expected", test);
  } else {
    LogMessage("ERROR! %s is false instead of true", test);
  }
}

// Log results of a string comparison for `test`.
static void ExpectStringsEqual(const char* test, const char* expected,
                               const char* actual) {
  if (strcmp(expected, actual) == 0) {
    LogMessage("%s is '%s' as expected", test, actual);
  } else {
    LogMessage("ERROR! %s is '%s' instead of '%s'", test, actual, expected);
  }
}

// NOTE: Don't use this sort of platform-specific logic in your applications.
// TODO(jsanmiya): All errors should be the same, regardless of platform.
// Remove when standardized.
inline AuthError TEMP_PlatformSpecificError(AuthError android_error,
                                            AuthError ios_error) {
#if defined(__ANDROID__)
  (void)ios_error;
  return android_error;
#else   // !defined(__ANDROID__)
  (void)android_error;
  return ios_error;
#endif  // !defined(__ANDROID__)
}

// Execute all methods of the C++ Auth API.
extern "C" int common_main(int argc, const char* argv[]) {
  LogMessage("Starting Auth tests.");

// Create the App wrapper.
#if defined(__ANDROID__)
  g_app = App::Create(AppOptions(FIREBASE_TESTAPP_NAME), GetJniEnv(),
                      GetActivity());
#else
  g_app = App::Create(AppOptions(FIREBASE_TESTAPP_NAME));
#endif  // defined(__ANDROID__)
  LogMessage("Created the Firebase app %x.",
             static_cast<int>(reinterpret_cast<intptr_t>(g_app)));

  // --- Auth tests ------------------------------------------------------------
  // Create the Auth class for that App.
  Auth* auth = Auth::GetAuth(g_app);
  LogMessage("Created the Auth %x class for the Firebase app.",
             static_cast<int>(reinterpret_cast<intptr_t>(auth)));

  // Test that CurrentUser() returns NULL right after creation.
  if (auth->CurrentUser() != NULL) {
    LogMessage("ERROR: CurrentUser() returning %x instead of NULL",
               auth->CurrentUser());
  }

  // Test Auth::FetchProvidersForEmail().
  Future<Auth::FetchProvidersResult> providers_future =
      auth->FetchProvidersForEmail(kTestEmail);
  WaitForFuture(providers_future, "Auth::FetchProvidersForEmail()",
                kAuthError_None);
  const Auth::FetchProvidersResult* pro = providers_future.Result();
  LogMessage("  email %s, num providers %d", pro->email.c_str(),
             pro->providers.size());
  for (auto it = pro->providers.begin(); it != pro->providers.end(); ++it) {
    LogMessage("    * %s", it->c_str());
  }

  // Test Auth::SignInAnonymously().
  Future<User*> sign_in_future = auth->SignInAnonymously();
  WaitForSignInFuture(sign_in_future, "Auth::SignInAnonymously()",
                      kAuthError_None, auth);

  // Test SignOut() after signed in anonymously.
  auth->SignOut();
  if (auth->CurrentUser() != NULL) {
    LogMessage(
        "ERROR: CurrentUser() returning %x instead of NULL after SignOut()",
        auth->CurrentUser());
  }

  // Test Auth::SignInWithEmailAndPassword().
  // Sign in with email and password that have already been registered.
  Future<User*> sign_in_future_ok =
      auth->SignInWithEmailAndPassword(kTestEmail, kTestPassword);
  WaitForSignInFuture(
      sign_in_future_ok,
      "Auth::SignInWithEmailAndPassword() existing email and password",
      kAuthError_None, auth);

  // Test SignOut() after signed in with email and password.
  auth->SignOut();
  if (auth->CurrentUser() != NULL) {
    LogMessage(
        "ERROR: CurrentUser() returning %x instead of NULL after SignOut()",
        auth->CurrentUser());
  }

  // Sign in user with bad email. Should fail.
  Future<User*> sign_in_future_bad_email =
      auth->SignInWithEmailAndPassword(kTestEmailBad, kTestPassword);
  WaitForSignInFuture(sign_in_future_bad_email,
                      "Auth::SignInWithEmailAndPassword() bad email",
                      firebase::kAuthError_InvalidEmail, auth);

  // Sign in user with correct email but bad password. Should fail.
  Future<User*> sign_in_future_bad_password =
      auth->SignInWithEmailAndPassword(kTestEmail, kTestPasswordBad);
  WaitForSignInFuture(sign_in_future_bad_password,
                      "Auth::SignInWithEmailAndPassword() bad password",
                      firebase::kAuthError_InvalidPassword, auth);

  // Test Auth::CreateUserWithEmailAndPassword().
  // Create user with random email and password. Should succeed.
  const std::string new_email = CreateNewEmail();
  Future<User*> create_future_ok =
      auth->CreateUserWithEmailAndPassword(new_email.c_str(), kTestPassword);
  WaitForSignInFuture(create_future_ok,
                      "Auth::CreateUserWithEmailAndPassword() new email",
                      kAuthError_None, auth);

  // Try to create with existing email. Should fail.
  Future<User*> create_future_bad =
      auth->CreateUserWithEmailAndPassword(kTestEmail, kTestPassword);
  WaitForSignInFuture(create_future_bad,
                      "Auth::CreateUserWithEmailAndPassword() existing email",
                      firebase::kAuthError_EmailExists, auth);

  // Test Auth::SignInWithCredential() using email&password.
  // Use existing email. Should succeed.
  AuthCredential email_cred_ok =
      EmailAuthProvider::GetCredential(g_app, kTestEmail, kTestPassword);
  Future<User*> sign_in_cred_ok = auth->SignInWithCredential(email_cred_ok);
  WaitForSignInFuture(sign_in_cred_ok,
                      "Auth::SignInWithCredential() existing email",
                      firebase::kAuthError_None, auth);

  // Use bad email. Should fail.
  AuthCredential email_cred_bad =
      EmailAuthProvider::GetCredential(g_app, kTestEmailBad, kTestPassword);
  Future<User*> sign_in_cred_bad = auth->SignInWithCredential(email_cred_bad);
  WaitForSignInFuture(sign_in_cred_bad,
                      "Auth::SignInWithCredential() bad email",
                      firebase::kAuthError_InvalidEmail, auth);

  // Use bad Facebook credentials. Should fail.
  // TODO(jsanmiya): Also add good Facebook credentials.
  AuthCredential facebook_cred_bad =
      FacebookAuthProvider::GetCredential(g_app, kTestAccessTokenBad);
  Future<User*> facebook_bad = auth->SignInWithCredential(facebook_cred_bad);
  WaitForSignInFuture(
      facebook_bad, "Auth::SignInWithCredential() bad Facebook credentials",
      TEMP_PlatformSpecificError(firebase::kAuthError_GeneralBackendError,
                                 firebase::kAuthError_InternalError),
      auth);

  // Use bad Google credentials. Should fail.
  // TODO(jsanmiya): Also add good Google credentials.
  AuthCredential google_cred_bad = GoogleAuthProvider::GetCredential(
      g_app, kTestIdTokenBad, kTestAccessTokenBad);
  Future<User*> google_bad = auth->SignInWithCredential(google_cred_bad);
  WaitForSignInFuture(
      google_bad, "Auth::SignInWithCredential() bad Google credentials",
      TEMP_PlatformSpecificError(firebase::kAuthError_GeneralBackendError,
                                 firebase::kAuthError_InternalError),
      auth);

  // Test Auth::SendPasswordResetEmail().
  // Use existing email. Should succeed.
  Future<void> send_password_reset_ok =
      auth->SendPasswordResetEmail(kTestEmail);
  WaitForFuture(send_password_reset_ok,
                "Auth::SendPasswordResetEmail() existing email",
                firebase::kAuthError_None);

  // Use bad email. Should fail.
  Future<void> send_password_reset_bad =
      auth->SendPasswordResetEmail(kTestEmailBad);
  WaitForFuture(send_password_reset_bad,
                "Auth::SendPasswordResetEmail() bad email",
                TEMP_PlatformSpecificError(firebase::kAuthError_EmailNotFound,
                                           firebase::kAuthError_InvalidEmail));

  // Test unimplemented APIs, just to ensure they don't crash.
  Future<User*> confirm_password_reset =
      auth->ConfirmPasswordReset(kTestEmail, kTestCodeBad);
  WaitForFuture(confirm_password_reset, "Auth::ConfirmPasswordReset()",
                firebase::kAuthError_Unimplemented);

  Future<Auth::ActionCodeResult> check_action_code =
      auth->CheckActionCode(kTestCodeBad);
  WaitForFuture(check_action_code, "Auth::CheckActionCode()",
                firebase::kAuthError_Unimplemented);

  Future<void> apply_action_code = auth->ApplyActionCode(kTestCodeBad);
  WaitForFuture(apply_action_code, "Auth::ApplyActionCode()",
                firebase::kAuthError_Unimplemented);

  // --- User tests ------------------------------------------------------------
  // Test anonymous user info strings.
  Future<User*> anon_sign_in_for_user = auth->SignInAnonymously();
  WaitForSignInFuture(anon_sign_in_for_user,
                      "Auth::SignInAnonymously() for User", kAuthError_None,
                      auth);
  User* anonymous_user = *anon_sign_in_for_user.Result();
  if (anonymous_user != nullptr) {
    LogMessage("Anonymous UId is %s", anonymous_user->UId().c_str());
    ExpectStringsEqual("Anonymous user Email", "",
                       anonymous_user->Email().c_str());
    ExpectStringsEqual("Anonymous user DisplayName", "",
                       anonymous_user->DisplayName().c_str());
    ExpectStringsEqual("Anonymous user PhotoUrl", "",
                       anonymous_user->PhotoUrl().c_str());
    ExpectStringsEqual("Anonymous user ProviderId", kFirebaseProviderId,
                       anonymous_user->ProviderId().c_str());
    ExpectTrue("Anonymous email Anonymous()", anonymous_user->Anonymous());

    // Test User::LinkWithCredential().
    const std::string newer_email = CreateNewEmail();
    AuthCredential user_cred = EmailAuthProvider::GetCredential(
        g_app, newer_email.c_str(), kTestPassword);
    Future<User*> link_future = anonymous_user->LinkWithCredential(user_cred);
    WaitForSignInFuture(link_future, "User::LinkWithCredential()",
                        kAuthError_None, auth);

    // Test email user info strings.
    Future<User*> email_sign_in_for_user =
        auth->SignInWithCredential(user_cred);
    WaitForSignInFuture(email_sign_in_for_user,
                        "Auth::SignInWithCredential() for User",
                        kAuthError_None, auth);
    User* email_user = *email_sign_in_for_user.Result();
    if (email_user != nullptr) {
      LogMessage("Email UId is %s", email_user->UId().c_str());
      ExpectStringsEqual("Email user Email", newer_email.c_str(),
                         email_user->Email().c_str());
      ExpectStringsEqual("Email user DisplayName", "",
                         email_user->DisplayName().c_str());
      ExpectStringsEqual("Email user PhotoUrl", "",
                         email_user->PhotoUrl().c_str());
      ExpectStringsEqual("Email user ProviderId", kFirebaseProviderId,
                         email_user->ProviderId().c_str());
      ExpectFalse("Email email Anonymous()", email_user->Anonymous());

      // Test UpdateUserProfile().
      User::UserProfile profile;
      profile.display_name = kTestDisplayName;
      profile.photo_uri = kTestPhotoUri;
      Future<void> user_profile_future = email_user->UpdateUserProfile(profile);
      WaitForFuture(user_profile_future, "User::UpdateUserProfile()",
                    kAuthError_None);
      ExpectStringsEqual("Updated DisplayName", kTestDisplayName,
                         email_user->DisplayName().c_str());
      ExpectStringsEqual("Updated PhotoUrl", kTestPhotoUri,
                         email_user->PhotoUrl().c_str());

      // Test User::Token().
      // with force_refresh = false.
      Future<std::string> token_no_refresh = email_user->Token(false);
      WaitForFuture(token_no_refresh, "User::Token(false)", kAuthError_None);
      LogMessage("User::Token(false) = %s", token_no_refresh.Result()->c_str());

      // with force_refresh = true.
      Future<std::string> token_force_refresh = email_user->Token(true);
      WaitForFuture(token_force_refresh, "User::Token(true)", kAuthError_None);
      LogMessage("User::Token(true) = %s",
                 token_force_refresh.Result()->c_str());

      // Test Reload().
      Future<void> reload_future = email_user->Reload();
      WaitForFuture(reload_future, "User::Reload()", kAuthError_None);

      // Test User::RefreshToken().
      const std::string refresh_token = email_user->RefreshToken();
      LogMessage("User::RefreshToken() = %s", refresh_token.c_str());

      // Test User::Unlink().
      Future<User*> unlink_future = email_user->Unlink("firebase");
      WaitForSignInFuture(
          unlink_future, "User::Unlink()",
          TEMP_PlatformSpecificError(firebase::kAuthError_EmailSignUpNotAllowed,
                                     firebase::kAuthError_NoSuchProvider),
          auth);

      // Sign in again if user is now invalid.
      // TODO(jsanmiya): User should not be invalid on bad Unlink. Remove this
      // when iOS implementation is fixed.
      if (auth->CurrentUser() == nullptr) {
        Future<User*> email_sign_in_again =
            auth->SignInWithCredential(user_cred);
        WaitForSignInFuture(email_sign_in_again,
                            "Auth::SignInWithCredential() again",
                            kAuthError_None, auth);
        email_user = *email_sign_in_again.Result();
      }
    }
    if (email_user != nullptr) {
      // Test User::ProviderData().
      const std::vector<UserInfoInterface*>& provider_data =
          email_user->ProviderData();
      LogMessage("User::ProviderData() returned %d interface%s",
                 provider_data.size(), provider_data.size() == 1 ? "" : "s");
      for (size_t i = 0; i < provider_data.size(); ++i) {
        const UserInfoInterface* user_info = provider_data[i];
        LogMessage(
            "    UId() = %s\n"
            "    Email() = %s\n"
            "    DisplayName() = %s\n"
            "    PhotoUrl() = %s\n"
            "    ProviderId() = %s",
            user_info->UId().c_str(), user_info->Email().c_str(),
            user_info->DisplayName().c_str(), user_info->PhotoUrl().c_str(),
            user_info->ProviderId().c_str());
      }

      // Test User::UpdateEmail().
      const std::string newest_email = CreateNewEmail();
      Future<void> update_email_future =
          email_user->UpdateEmail(newest_email.c_str());
      WaitForFuture(update_email_future, "User::UpdateEmail()",
                    kAuthError_None);

      // Test User::UpdateEmail().
      Future<void> update_password_future =
          email_user->UpdatePassword(kTestPasswordUpdated);
      WaitForFuture(update_password_future, "User::UpdatePassword()",
                    kAuthError_None);

      // Test User::Reauthenticate().
      // TODO(jsanmiya): When implemented change to WaitForSignInFuture().
      AuthCredential email_cred_reauth = EmailAuthProvider::GetCredential(
          g_app, newest_email.c_str(), kTestPasswordUpdated);
      Future<User*> reauthenticate_future =
          email_user->Reauthenticate(email_cred_reauth);
      WaitForFuture(reauthenticate_future, "User::Reauthenticate()",
                    firebase::kAuthError_Unimplemented);

      // Test User::SendEmailVerification().
      Future<void> send_email_verification_future =
          email_user->SendEmailVerification();
      WaitForFuture(
          send_email_verification_future, "User::SendEmailVerification()",
          TEMP_PlatformSpecificError(firebase::kAuthError_Unimplemented,
                                     firebase::kAuthError_InternalError));

      // Test User::Delete().
      Future<void> delete_future = email_user->Delete();
      WaitForFuture(
          delete_future, "User::Delete()",
          TEMP_PlatformSpecificError(firebase::kAuthError_Unimplemented,
                                     firebase::kAuthError_TokenExpired));
    }
  }

  LogMessage("Completed Auth tests.");
  return 0;
}
