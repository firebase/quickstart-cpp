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

package com.google.firebase.example;

import android.app.Activity;
import android.graphics.Typeface;
import android.os.Handler;
import android.os.Looper;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.MotionEvent;
import android.view.View;
import android.view.Window;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

/**
 * A utility class, encapsulating the data and methods required to log arbitrary
 * text to the screen, via a non-editable TextView.
 */
public class LoggingUtils {
  static TextView textView = null;
  static ScrollView scrollView = null;
  // Tracks if the log window was touched at least once since the testapp was started.
  static boolean didTouch = false;

  public static void initLogWindow(Activity activity) {
    initLogWindow(activity, true);
  }

  public static void initLogWindow(Activity activity, boolean monospace) {
    LinearLayout linearLayout = new LinearLayout(activity);
    scrollView = new ScrollView(activity);
    textView = new TextView(activity);
    textView.setTag("Logger");
    if (monospace) {
      textView.setTypeface(Typeface.MONOSPACE);
      textView.setTextSize(10);
    }
    linearLayout.addView(scrollView);
    scrollView.addView(textView);
    Window window = activity.getWindow();
    window.takeSurface(null);
    window.setContentView(linearLayout);

    // Force the TextView to stay scrolled to the bottom.
    textView.addTextChangedListener(
        new TextWatcher() {
          @Override
          public void afterTextChanged(Editable e) {
            if (scrollView != null && !didTouch) {
              // If the user never interacted with the screen, scroll to bottom.
              scrollView.fullScroll(View.FOCUS_DOWN);
            }
          }
          @Override
          public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

          @Override
          public void onTextChanged(CharSequence s, int start, int count, int after) {}
        });
    textView.setOnTouchListener(
        new View.OnTouchListener() {
          @Override
          public boolean onTouch(View v, MotionEvent event) {
            didTouch = true;
            return false;
          }
        });
  }

  public static void addLogText(final String text) {
    new Handler(Looper.getMainLooper())
        .post(
            new Runnable() {
              @Override
              public void run() {
                if (textView != null) {
                  textView.append(text);
                }
              }
            });
  }

  public static boolean getDidTouch() {
    return didTouch;
  }
}
