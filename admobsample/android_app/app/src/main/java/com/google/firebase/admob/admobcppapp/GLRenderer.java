// Copyright 2016 Google Inc. All Rights Reserved.

package com.google.firebase.admob.admobcppapp;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.opengl.GLSurfaceView.Renderer;

public class GLRenderer implements Renderer {

    public GLRenderer() {
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        nativeOnSurfaceCreated();
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        nativeOnSurfaceChanged(width, height);
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        nativeOnUpdate();
        nativeOnDrawFrame();
    }

    private native void nativeOnSurfaceCreated();

    private native void nativeOnSurfaceChanged(int width, int height);

    private native void nativeOnDrawFrame();

    private native void nativeOnUpdate();
}
