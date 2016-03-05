// Copyright 2016 Google Inc. All Rights Reserved.

package com.google.android.gms.ads.cpphelpers;

import android.app.Activity;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.widget.PopupWindow;

import com.google.android.gms.ads.AdListener;
import com.google.android.gms.ads.AdRequest;
import com.google.android.gms.ads.AdSize;
import com.google.android.gms.ads.AdView;

import java.util.Calendar;
import java.util.Date;

public class BannerViewHelper {

    private static final String LOG_TAG = "BannerViewHelper";

    // Lifecycle states (matches enumeration on C++ side)
    public final int LIFECYCLE_STATE_INITIALIZING = 0;
    public final int LIFECYCLE_STATE_INITIALIZED = 1;
    public final int LIFECYCLE_STATE_LOADING = 2;
    public final int LIFECYCLE_STATE_LOADED = 3;
    public final int LIFECYCLE_STATE_FAILEDINTERNALERROR = 4;
    public final int LIFECYCLE_STATE_FAILEDINVALIDREQUEST = 5;
    public final int LIFECYCLE_STATE_FAILEDNETWORKERROR = 6;
    public final int LIFECYCLE_STATE_FAILEDNOFILL = 7;
    public final int LIFECYCLE_STATE_DESTROYED = 8;
    public final int LIFECYCLE_STATE_FATALERROR = 9;

    // Presentation states (matches enumeration on C++ side)
    public final int PRESENTATION_STATE_HIDDEN = 0;
    public final int PRESENTATION_STATE_VISIBLEWITHOUTAD = 1;
    public final int PRESENTATION_STATE_VISIBLEWITHAD = 2;
    public final int PRESENTATION_STATE_OPENEDPARTIALOVERLAY = 3;
    public final int PRESENTATION_STATE_COVERINGUI = 4;

    // The pre-defined BannerView positions
    public final int POSITION_TOP = 0;
    public final int POSITION_BOTTOM = 1;
    public final int POSITION_TOP_LEFT = 2;
    public final int POSITION_TOP_RIGHT = 3;
    public final int POSITION_BOTTOM_LEFT = 4;
    public final int POSITION_BOTTOM_RIGHT = 5;

    private int mCurrentLifecycleState;
    private boolean mAdViewContainsAd;
    private Activity mActivity;
    private AdSize mAdSize;
    private AdView mAdView;
    private PopupWindow mPopUp;
    private String mAdUnitId;
    private boolean mShouldUseXYForPosition;
    private int mDesiredPosition;
    private int mDesiredX;
    private int mDesiredY;

    // Do nothing.
    public BannerViewHelper(Activity activity) {
        Log.d(LOG_TAG, "instantiating");
        mCurrentLifecycleState = LIFECYCLE_STATE_INITIALIZING;
        mAdViewContainsAd = false;
        mActivity = activity;
        mAdSize = AdSize.BANNER;
        mAdUnitId = "ca-app-pub-3940256099942544/6300978111";
        mDesiredPosition = POSITION_BOTTOM;
        mShouldUseXYForPosition = false;

        // Create the PopUpWindow and AdView on the UI thread.
        mActivity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                initialize();
                mCurrentLifecycleState = LIFECYCLE_STATE_INITIALIZED;
            }
        });
    }

    // Do nothing.
    public BannerViewHelper(Activity activity, String adUnitID, AdSize adSize) {

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
        mCurrentLifecycleState = LIFECYCLE_STATE_DESTROYED;
        mActivity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (mAdView != null); {
                    mAdView.destroy();
                    mAdView = null;
                }

                if (mPopUp != null) {
                    mPopUp.dismiss();
                    mPopUp = null;
                }
            }
        });
    }

    public void loadAd(AdRequest request) {
        // TODO: Check lifecycle first
        mCurrentLifecycleState = LIFECYCLE_STATE_LOADING;
        mActivity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mAdView.loadAd(new AdRequest.Builder().build());
            }
        });
    }

    public void hide() {
        mActivity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mPopUp.dismiss();
            }
        });
    }

    public void show() {
        if (mPopUp != null) {
            displayPopUp();
        }
    }

    public void pause() {
        Log.d(LOG_TAG, "Pausing!");
        if (mAdView != null) {
            mActivity.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    mAdView.pause();
                }
            });
        }
    }

    public void resume() {
        Log.d(LOG_TAG, "Resuming!");
        if (mAdView != null) {
            mActivity.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    mAdView.resume();
                }
            });
        }
    }

    public void moveTo(final int x, final int y) {
        mDesiredX = x;
        mDesiredY = y;
        mShouldUseXYForPosition = true;

        if ((mPopUp != null) && (mPopUp.isShowing())) {
            displayPopUp();
        }
    }

    public void moveTo(final int position) {
        mShouldUseXYForPosition = false;
        mDesiredPosition = position;

        if ((mPopUp != null && mPopUp.isShowing())) {
            displayPopUp();
        }
    }

    private void displayPopUp() {
        mActivity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                View root = mActivity.findViewById(android.R.id.content).getRootView();
                mPopUp.dismiss();
                if (mShouldUseXYForPosition) {
                    mPopUp.showAtLocation(root, Gravity.NO_GRAVITY, mDesiredX, mDesiredY);
                } else {
                    switch (mDesiredPosition) {
                        case POSITION_TOP:
                            mPopUp.showAtLocation(root, Gravity.TOP | Gravity.CENTER_HORIZONTAL,
                                    0, 0);
                            break;
                        case POSITION_BOTTOM:
                            mPopUp.showAtLocation(root, Gravity.BOTTOM | Gravity.CENTER_HORIZONTAL,
                                    0, 0);
                            break;
                        case POSITION_TOP_LEFT:
                            mPopUp.showAtLocation(root, Gravity.TOP | Gravity.LEFT, 0, 0);
                            break;
                        case POSITION_TOP_RIGHT:
                            mPopUp.showAtLocation(root, Gravity.TOP | Gravity.RIGHT, 0, 0);
                            break;
                        case POSITION_BOTTOM_LEFT:
                            mPopUp.showAtLocation(root, Gravity.BOTTOM | Gravity.LEFT, 0, 0);
                            break;
                        case POSITION_BOTTOM_RIGHT:
                            mPopUp.showAtLocation(root, Gravity.BOTTOM | Gravity.RIGHT, 0, 0);
                            break;
                    }
                }
            }
        });
    }

    public int getWidth() {
        if (mPopUp != null && mAdView != null) {
            if (mPopUp.isShowing()) {
                return mAdView.getWidth();
            } else {
                return 0;
            }
        } else {
            Log.w(LOG_TAG, "getWidth() called with no popup. Was this BannerView destroyed?");
            return 0;
        }
    }

    public int getHeight() {
        if (mPopUp != null && mAdView != null) {
            if (mPopUp.isShowing()) {
                return mAdView.getHeight();
            } else {
                return 0;
            }
        } else {
            Log.w(LOG_TAG, "getHeight() called with no popup. Was this BannerView destroyed?");
            return 0;
        }
    }

    public int getX() {
        if (mPopUp != null) {
            int[] onScreenLocation = new int[2];
            mPopUp.getContentView().getLocationOnScreen(onScreenLocation);
            return onScreenLocation[0];
        } else {
            Log.w(LOG_TAG, "getX() called with no popup. Was this banner destroyed?");
            return 0;
        }
    }

    public int getY() {
        if (mPopUp != null) {
            int[] onScreenLocation = new int[2];
            mPopUp.getContentView().getLocationOnScreen(onScreenLocation);
            return onScreenLocation[1];
        } else {
            Log.w(LOG_TAG, "getY() called with no popup. Was this banner destroyed?");
            return 0;
        }
    }

    public int getLifecycleState() {
        return mCurrentLifecycleState;
    }

    public int getPresentationState() {
        return mPopUp.isShowing() ? PRESENTATION_STATE_VISIBLEWITHAD : PRESENTATION_STATE_HIDDEN;
    }

    private void initialize() {
        mAdView = new AdView(mActivity);
        mAdView.setAdUnitId(mAdUnitId);
        mAdView.setAdSize(mAdSize);
        mAdView.setAdListener(new AdListener() {
            @Override
            public void onAdClosed() {
                super.onAdClosed();
            }

            @Override
            public void onAdFailedToLoad(int errorCode) {
                switch (errorCode) {
                    case AdRequest.ERROR_CODE_INTERNAL_ERROR:
                        mCurrentLifecycleState = LIFECYCLE_STATE_FAILEDINTERNALERROR;
                        break;
                    case AdRequest.ERROR_CODE_INVALID_REQUEST:
                        mCurrentLifecycleState = LIFECYCLE_STATE_FAILEDINVALIDREQUEST;
                        break;
                    case AdRequest.ERROR_CODE_NETWORK_ERROR:
                        mCurrentLifecycleState = LIFECYCLE_STATE_FAILEDNETWORKERROR;
                        break;
                    case AdRequest.ERROR_CODE_NO_FILL:
                        mCurrentLifecycleState = LIFECYCLE_STATE_FAILEDNOFILL;
                        break;
                }

                super.onAdFailedToLoad(errorCode);
            }

            @Override
            public void onAdLeftApplication() {
                super.onAdLeftApplication();
            }

            @Override
            public void onAdLoaded() {
                mAdViewContainsAd = true;
                mCurrentLifecycleState = LIFECYCLE_STATE_LOADED;
                super.onAdLoaded();
            }

            @Override
            public void onAdOpened() {
                super.onAdOpened();
            }
        });

        mPopUp = new PopupWindow(mAdView, LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
    }
}
