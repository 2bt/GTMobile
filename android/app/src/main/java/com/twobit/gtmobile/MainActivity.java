package com.twobit.gtmobile;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.util.Log;
import android.view.inputmethod.InputMethodManager;
import android.view.KeyEvent;

public class MainActivity extends Activity {
    static final String TAG = "Main";
    static MainActivity sInstance;
    View mView;

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

    void loadSettings() {
        SharedPreferences prefs = getPreferences(MODE_PRIVATE);
        String name;
        for (int i = 0; (name = Native.getValueName(i)) != null; ++i) {
            int v = Native.getValue(i);
            v = prefs.getInt(name, v);
            Native.setValue(i, v);
        }
    }
    void saveSettings() {
        SharedPreferences prefs = getPreferences(MODE_PRIVATE);
        SharedPreferences.Editor edit = prefs.edit();
        String name;
        for (int i = 0; (name = Native.getValueName(i)) != null; ++i) {
            int v = Native.getValue(i);
            edit.putInt(name, v);
        }
        edit.apply();
    }

    public MainActivity() {
        sInstance = this;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        loadSettings();
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
        saveSettings();
    }

    @Override
    public boolean onKeyDown(final int code, final KeyEvent e) {
        mView.queueEvent(new Runnable() { public void run() {
            Native.key(code, e.getUnicodeChar());
        }});
        return false;
    }
}
