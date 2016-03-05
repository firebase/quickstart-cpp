// Copyright 2016 Google Inc. All Rights Reserved.

package com.google.android.gms.ads.cpphelpers;

import com.google.android.gms.ads.AdListener;
import com.google.android.gms.ads.AdLoader;
import com.google.android.gms.ads.AdRequest;
import com.google.android.gms.ads.AdSize;
import com.google.android.gms.ads.InterstitialAd;

import android.app.Activity;
import android.util.Log;

import java.lang.String;
import java.util.Calendar;
import java.util.Date;

public class InterstitialAdHelper {

    private static final String LOG_TAG = "InterstitialAdHelper";

    // Lifecycle states (matches enumeration on C++ side)
    public final int LIFECYCLE_STATE_INITIALIZING = 0;
    public final int LIFECYCLE_STATE_INITIALIZED = 1;
    public final int LIFECYCLE_STATE_LOADING = 2;
    public final int LIFECYCLE_STATE_LOADED = 3;
    public final int LIFECYCLE_STATE_HAS_SHOWN_AD = 4;
    public final int LIFECYCLE_STATE_FAILED_INTERNAL_ERROR = 5;
    public final int LIFECYCLE_STATE_FAILED_INVALID_REQUEST = 6;
    public final int LIFECYCLE_STATE_FAILED_NETWORK_ERROR = 7;
    public final int LIFECYCLE_STATE_FAILED_NO_FILL = 8;
    public final int LIFECYCLE_STATE_FATAL_ERROR = 9;

    // Presentation states (matches enumeration on C++ side)
    public final int PRESENTATION_STATE_HIDDEN = 0;
    public final int PRESENTATION_STATE_COVERING_UI = 1;

    private InterstitialAd mInterstitial;
    private int mCurrentLifecycleState;
    private Activity mActivity;
    private String mAdUnitId;

    public InterstitialAdHelper(Activity activity, String adUnitID) {
        mCurrentLifecycleState = LIFECYCLE_STATE_INITIALIZING;
        mActivity = activity;
        mAdUnitId = adUnitID;

        mActivity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                initialize();
                mCurrentLifecycleState = LIFECYCLE_STATE_INITIALIZED;
            }
        });
    }

    public Date createDate(int year, int month, int day) {
        try {
            Calendar cal = Calendar.getInstance();
            cal.setLenient(false);
            cal.set(year, month - 1, day);
            return cal.getTime();
        } catch (IllegalArgumentException e) {
            Log.w(LOG_TAG, String.format("Invalid date entered (m/d/y = %d/%d/%d).", month, day,
                    year));
            return null;
        }
    }

    public void destroy() {
    }

    public void loadAd(AdRequest request) {
        // TODO: Check lifecycle first
        mCurrentLifecycleState = LIFECYCLE_STATE_LOADING;
        mActivity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mInterstitial.loadAd(new AdRequest.Builder().build());
            }
        });
    }

    public void show() {
        // TODO: Check lifecycle first
        mActivity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (mInterstitial.isLoaded()) {
                    mInterstitial.show();
                }
            }
        });
    }

    public int getLifecycleState() {
        return 0;
    }

    public int getPresentationState() {
        return 0;
    }

    private void initialize() {
        mInterstitial = new InterstitialAd(mActivity);
        mInterstitial.setAdUnitId(mAdUnitId);
        mInterstitial.setAdListener(new AdListener() {
            @Override
            public void onAdClosed() {
                super.onAdClosed();
            }

            @Override
            public void onAdFailedToLoad(int errorCode) {
                super.onAdFailedToLoad(errorCode);
            }

            @Override
            public void onAdLeftApplication() {
                super.onAdLeftApplication();
            }

            @Override
            public void onAdLoaded() {
                super.onAdLoaded();
            }

            @Override
            public void onAdOpened() {
                super.onAdOpened();
            }
        });
    }
}
