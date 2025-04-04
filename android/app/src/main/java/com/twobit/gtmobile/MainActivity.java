package com.twobit.gtmobile;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.Window;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.view.KeyEvent;
import android.media.midi.MidiManager;
import android.media.midi.MidiDeviceInfo;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.app.ActivityCompat;

import java.io.File;


public class MainActivity extends Activity {
    private static final String TAG = "Main";
    private static final int NOTIFICATION_PERMISSION_CODE = 1;
    private static final int REQUEST_CODE_IMPORT_FILE = 1;
    private static final int REQUEST_CODE_EXPORT_FILE = 2;

    private static final int SETTING_FULLSCREEN_ENABLED = 0;
    private static final int SETTING_KEEP_SCREEN_ON = 1;


    private static MainActivity sInstance;
    private String mExportPath;
    private boolean mExportDeleteWhenDone;
    private View mView;


    // called from C++
    static public void showKeyboard(boolean show) {
        sInstance.runOnUiThread(() -> {
            InputMethodManager imm = (InputMethodManager) sInstance.getSystemService(Context.INPUT_METHOD_SERVICE);
            if (show) {
                imm.showSoftInput(sInstance.getWindow().getDecorView(), InputMethodManager.SHOW_IMPLICIT);
            } else {
                imm.hideSoftInputFromWindow(sInstance.getWindow().getDecorView().getWindowToken(), 0);
            }
        });
    }

    // called from C++
    static public void startSongImport() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        sInstance.startActivityForResult(intent, REQUEST_CODE_IMPORT_FILE);
    }

    // called from C++
    static public void exportFile(String path, String title, boolean deleteWhenDone) {
        Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        intent.putExtra(Intent.EXTRA_TITLE, title);
        sInstance.mExportPath = path;
        sInstance.mExportDeleteWhenDone = deleteWhenDone;
        sInstance.startActivityForResult(intent, REQUEST_CODE_EXPORT_FILE);
    }

    // called from C++
    static public void updateSetting(int i) {
        int v = Native.getSettingValue(i);
        sInstance.runOnUiThread(() -> {
            switch (i) {
                case SETTING_FULLSCREEN_ENABLED:
                    sInstance.setImmersiveMode(v != 0);
                    break;
                case SETTING_KEEP_SCREEN_ON:
                    sInstance.mView.setKeepScreenOn(v != 0);
                    break;
                default:
                    break;
            }
        });
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        Log.i(TAG, "onActivityResult " + requestCode + " " + resultCode);
        if (resultCode != Activity.RESULT_OK || data == null) return;
        Uri fileUri = data.getData();

        if (requestCode == REQUEST_CODE_IMPORT_FILE) {
            String fileName = FileUtils.getFileName(this, fileUri);
            if (fileName == null) fileName = "song.sng";
            String path = getCacheDir() + "/" + fileName;
            FileUtils.copyUriToFile(this, fileUri, path);
            Native.importSong(path);
            new File(path).delete();
        }
        if (requestCode == REQUEST_CODE_EXPORT_FILE) {
            FileUtils.copyFileToUri(this, mExportPath, fileUri);
            if (mExportDeleteWhenDone) new File(mExportPath).delete();
        }
    }


    private void loadSettings() {
        Log.i(TAG, "loadSettings");
        SharedPreferences prefs = getPreferences(MODE_PRIVATE);
        String name;
        for (int i = 0; (name = Native.getSettingName(i)) != null; ++i) {
            int v = Native.getSettingValue(i);
            v = prefs.getInt(name, v);
            Native.setSettingValue(i, v);
            Log.i(TAG, "loadSettings: " + name + " = " + v);
        }
    }
    private void saveSettings() {
        SharedPreferences prefs = getPreferences(MODE_PRIVATE);
        SharedPreferences.Editor edit = prefs.edit();
        String name;
        for (int i = 0; (name = Native.getSettingName(i)) != null; ++i) {
            int v = Native.getSettingValue(i);
            edit.putInt(name, v);
        }
        edit.apply();
    }

    public MainActivity() {
        sInstance = this;
    }

    private void setImmersiveMode(boolean enabled) {
        Window window = getWindow();
        if (enabled) {
            window.getDecorView().setSystemUiVisibility(
                    View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_FULLSCREEN
                            | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
            );
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
                WindowManager.LayoutParams layoutParams = window.getAttributes();
                layoutParams.layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
                window.setAttributes(layoutParams);
            }
        } else {
            window.getDecorView().setSystemUiVisibility(
                    View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                            | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
            );
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
                WindowManager.LayoutParams layoutParams = window.getAttributes();
                layoutParams.layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_NEVER;
                window.setAttributes(layoutParams);
            }
        }
    }

    private void openFirstMidiDevice() {
        MidiManager midiManager = (MidiManager) getApplicationContext().getSystemService(Context.MIDI_SERVICE);
        MidiDeviceInfo[] devices = midiManager.getDevices();
        Log.i(TAG, "found " + devices.length + " MIDI devices");
        if (devices.length == 0) return;
        midiManager.openDevice(devices[0], device -> {
            if (device != null) Native.setMidiDevice(device);
        }, null);
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

        // apply settings
        updateSetting(SETTING_FULLSCREEN_ENABLED);
        updateSetting(SETTING_KEEP_SCREEN_ON);

        openFirstMidiDevice();
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
        mView.queueEvent(() -> {
            Native.key(code, e.getUnicodeChar());
        });
        return false;
    }
}
