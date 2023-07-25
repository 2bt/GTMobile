package com.twobit.gtmobile;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.util.Log;
import android.view.inputmethod.InputMethodManager;
import android.view.KeyEvent;

public class MainActivity extends Activity {
    static final String TAG = "Main";
    View mView;
    static MainActivity sInstance;

    // called from C++
    static public void showKeyboard(boolean show) {
        sInstance.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                InputMethodManager imm = (InputMethodManager) sInstance.getSystemService(Context.INPUT_METHOD_SERVICE);
                if (show) {
                    imm.showSoftInput(sInstance.getWindow().getDecorView(), InputMethodManager.SHOW_IMPLICIT);
                } else {
                    imm.hideSoftInputFromWindow(sInstance.getWindow().getDecorView().getWindowToken(), 0);
                }
            }
        });
    }

    public MainActivity() {
        sInstance = this;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mView = new View(getApplication());
        setContentView(mView);
    }

    @Override
    protected void onResume() {
        super.onResume();
        mView.onResume();
        Native.startAudio();
    }

    @Override
    protected void onPause() {
        super.onPause();
        mView.onPause();
        Native.stopAudio();
    }

    @Override
    public boolean onKeyDown(final int code, final KeyEvent e) {
        mView.queueEvent(new Runnable() { public void run() {
            Native.key(code, e.getUnicodeChar());
        }});
        return false;
    }
}
