// Copyright 2016 Google Inc. All Rights Reserved.

package com.google.firebase.example;

import android.app.NativeActivity;
import android.content.Intent;

/**
 * TestappNativeActivity is a NativeActivity that updates its intent when new intents are sent to
 * it.
 *
 * This is a workaround for a known issue that prevents the native C++ library from responding to
 * data payloads when both a data and notification payload are sent to the app while it is in the
 * background.
 */
public class TestappNativeActivity extends NativeActivity {
  /**
   * Workaround for when a message is sent containing both a Data and Notification payload.
   *
   * When the app is in the foreground all data payloads are sent to the method
   * `::firebase::messaging::Listener::OnMessage`. However, when the app is in the background, if a
   * message with both a data and notification payload is receieved the data payload is stored on
   * the notification Intent. NativeActivity does not provide native callbacks for onNewIntent, so
   * it cannot route the data payload that is stored in the Intent to the C++ function OnMessage. As
   * a workaround, we override onNewIntent so that it sets the Activity's intent, allowing the
   * native C++ code to get access to the data payload in the Intent extras and call OnMessage.
   */
  @Override
  protected void onNewIntent(Intent intent) {
    setIntent(intent);
  }
}
