package com.twobit.gtmobile;

import android.content.res.AssetManager;

class Native {
    static {
        System.loadLibrary("native");
    }
    public static native void init(AssetManager assetManager, float refreshRate);
    public static native void free();
    public static native void resize(int width, int height);
    public static native void draw();
    public static native void touch(int x, int y, int action);
    public static native void key(int key, int unicode);
    public static native void startAudio();
    public static native void stopAudio();
}
