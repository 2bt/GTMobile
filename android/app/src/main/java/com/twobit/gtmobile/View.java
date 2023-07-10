package com.twobit.gtmobile;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.view.MotionEvent;
import android.util.Log;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;


public class View extends GLSurfaceView {
    static final String TAG = "View";

    public View(Context context) {
        super(context);
        setPreserveEGLContextOnPause(true);
        setEGLContextClientVersion(2);
        setEGLConfigChooser(8, 8, 8, 0, 0, 0);
        setRenderer(new GLSurfaceView.Renderer() {
            @Override
            public void onSurfaceCreated(GL10 gl, EGLConfig config) {
                Native.init(getResources().getAssets());
            }
            @Override
            public void onSurfaceChanged(GL10 gl, int width, int height) {
                Native.resize(width, height);
            }
            @Override
            public void onDrawFrame(GL10 gl) {
                Native.draw();
            }
        });
    }

    @Override
    public boolean onTouchEvent(MotionEvent e) {
        Log.i(TAG, "onTouchEvent" + e);
        return false;
    }
}
