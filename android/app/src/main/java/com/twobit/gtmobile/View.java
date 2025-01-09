package com.twobit.gtmobile;

import android.content.Context;
import android.hardware.display.DisplayManager;
import android.opengl.GLSurfaceView;
import android.os.Build;
import android.os.Environment;
import android.view.Display;
import android.view.MotionEvent;
import android.util.Log;

import java.io.File;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;


public class View extends GLSurfaceView {
    static final String TAG = "View";

    public View(Context context) {
        super(context);

        float refreshRate;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O_MR1) {
            DisplayManager displayManager = (DisplayManager) context.getSystemService(Context.DISPLAY_SERVICE);
            Display display = displayManager.getDisplay(Display.DEFAULT_DISPLAY);
            refreshRate = display.getRefreshRate();
        }
        else {
            refreshRate = 60.0f;
        }

        String storageDir = context.getExternalFilesDir(null).getAbsolutePath();

        setPreserveEGLContextOnPause(true);
        setEGLContextClientVersion(2);
        setEGLConfigChooser(8, 8, 8, 0, 0, 0);
        setRenderer(new GLSurfaceView.Renderer() {
            @Override
            public void onSurfaceCreated(GL10 gl, EGLConfig config) {
                Native.init(getResources().getAssets(), storageDir, refreshRate);
            }
            @Override
            public void onSurfaceChanged(GL10 gl, int width, int height) {
                Log.i(TAG, "onSurfaceChanged " + width + " " + height);
                gl.glViewport(0, 0, width, height);
                Native.resize(width, height);
            }
            @Override
            public void onDrawFrame(GL10 gl) {
                Native.draw();
            }
        });
    }


    int     mTouchId;
    boolean mTouchPressed;

    @Override
    public boolean onTouchEvent(MotionEvent e) {
        int index  = e.getActionIndex();
        int id     = e.getPointerId(index);
        int action = -1;

        switch (e.getActionMasked()) {
        case MotionEvent.ACTION_DOWN:
        case MotionEvent.ACTION_POINTER_DOWN:
            mTouchId = id;
            if (mTouchPressed) action = MotionEvent.ACTION_MOVE;
            else {
                action = MotionEvent.ACTION_DOWN;
                mTouchPressed = true;
            }
            break;
        case MotionEvent.ACTION_UP:
        case MotionEvent.ACTION_POINTER_UP:
            if (mTouchPressed && id == mTouchId) {
                action = MotionEvent.ACTION_UP;
                mTouchPressed = false;
            }
            break;
        case MotionEvent.ACTION_MOVE:
            if (!mTouchPressed) break;
            int count = e.getPointerCount();
            for (int p = 0; p < count; ++p) {
                if (e.getPointerId(p) == mTouchId) {
                    index = p;
                    action = MotionEvent.ACTION_MOVE;
                    break;
                }
            }
            break;
        }
        if (action == -1) return false;

        final int a = action;
        final int x = (int) e.getX(index);
        final int y = (int) e.getY(index);
        queueEvent(new Runnable() {
            public void run() {
                Native.touch(x, y, a);
            }
        });
        return true;
    }


}
