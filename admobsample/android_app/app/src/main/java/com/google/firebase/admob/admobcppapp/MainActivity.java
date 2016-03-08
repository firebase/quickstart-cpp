package com.google.firebase.admob.admobcppapp;

import android.app.Activity;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.view.MotionEvent;

public class MainActivity extends Activity {

    private GLSurfaceView mGlView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        System.loadLibrary("firebaseSample");
        initializeGameEngine();

        // Configure OpenGL view and renderer
        mGlView = (GLSurfaceView) findViewById(R.id.gl_surface_view);
        mGlView.setEGLContextClientVersion(2);
        mGlView.setRenderer(new GLRenderer());
    }

    @Override
    protected void onPause() {
        super.onPause();
        mGlView.onPause();
        pauseGameEngine();
    }

    @Override
    protected void onResume() {
        super.onResume();
        resumeGameEngine();
        mGlView.onResume();
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (event.getAction() == MotionEvent.ACTION_DOWN) {
            final float x = event.getX();
            final float y = event.getY();

            mGlView.queueEvent(new Runnable() {
                @Override
                public void run() {
                    registerTap(x, y);
                }
            });
        }

        return super.onTouchEvent(event);
    }

    private native void initializeGameEngine();

    private native void pauseGameEngine();

    private native void resumeGameEngine();

    private native void registerTap(float x, float y);

}
