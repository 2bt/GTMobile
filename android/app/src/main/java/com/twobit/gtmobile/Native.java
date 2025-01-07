package com.twobit.gtmobile;

import android.content.res.AssetManager;

class Native {
    static {
        System.loadLibrary("native");
    }
    public static native void init(AssetManager assetManager, String storageDir, float refreshRate);
    public static native void free();
    public static native void resize(int width, int height);
    public static native void draw();
    public static native void touch(int x, int y, int action);
    public static native void key(int key, int unicode);
    public static native void importSong(String path);

    public static native void setPlaying(boolean stream, boolean player);
    public static native boolean isStreamPlaying();
    public static native boolean isPlayerPlaying();
    public static native String getSongName();

    // settings
    public static native String getValueName(int i);
    public static native int getValue(int i);
    public static native void setValue(int i, int v);
}
