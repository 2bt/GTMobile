package com.twobit.gtmobile;

class Native {
    static {
        System.loadLibrary("native");
    }
    public static native void init();
}
