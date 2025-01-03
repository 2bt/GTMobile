package com.twobit.gtmobile;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.inputmethod.InputMethodManager;
import android.view.KeyEvent;

import androidx.annotation.NonNull;
import androidx.core.app.ActivityCompat;


public class MainActivity extends Activity {
    private static final String TAG = "Main";
    private static final int NOTIFICATION_PERMISSION_CODE = 1;
    private static MainActivity sInstance;
    private View mView;

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

    private void loadSettings() {
        SharedPreferences prefs = getPreferences(MODE_PRIVATE);
        String name;
        for (int i = 0; (name = Native.getValueName(i)) != null; ++i) {
            int v = Native.getValue(i);
            v = prefs.getInt(name, v);
            Native.setValue(i, v);
        }
    }
    private void saveSettings() {
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
        Log.i(TAG, "onCreate");
        super.onCreate(savedInstanceState);

        loadSettings();
        mView = new View(getApplication());
        setContentView(mView);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            if (ActivityCompat.checkSelfPermission(this, android.Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(this, new String[]{ android.Manifest.permission.POST_NOTIFICATIONS }, NOTIFICATION_PERMISSION_CODE);
            }
        }
    }


    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        Log.i(TAG, "onRequestPermissionsResult");
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == NOTIFICATION_PERMISSION_CODE) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                Log.i(TAG, "Permission granted: " + requestCode);
            } else {
                Log.i(TAG, "Permission NOT granted: " + requestCode);
            }
        }
    }

    @Override
    protected void onDestroy() {
        Log.i(TAG, "onDestroy");
        Intent serviceIntent = new Intent(getApplicationContext(), MusicService.class);
        stopService(serviceIntent);
        Native.setPlaying(false, false);
        Native.free();
        super.onDestroy();
    }

    @Override
    protected void onPause() {
        Log.i(TAG, "onPause");
        super.onPause();
        mView.onPause();
        saveSettings();
        if (!Native.isPlayerPlaying()) {
            Native.setPlaying(false, false);
        }

        Intent serviceIntent = new Intent(this, MusicService.class);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            startForegroundService(serviceIntent);
        } else {
            startService(serviceIntent);
        }
    }

    @Override
    protected void onResume() {
        Log.i(TAG, "onResume");
        super.onResume();
        mView.onResume();
        if (!Native.isStreamPlaying()) {
            Native.setPlaying(true, false);
        }
        Intent serviceIntent = new Intent(getApplicationContext(), MusicService.class);
        stopService(serviceIntent);
    }

    @Override
    public boolean onKeyDown(final int code, final KeyEvent e) {
        mView.queueEvent(new Runnable() { public void run() {
            Native.key(code, e.getUnicodeChar());
        }});
        return false;
    }
}
