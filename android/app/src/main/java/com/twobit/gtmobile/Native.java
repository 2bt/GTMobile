package com.twobit.gtmobile;

import android.content.res.AssetManager;

class Native {
    static {
        System.loadLibrary("native");
    }
    public static native void init(AssetManager assetManager);
}
