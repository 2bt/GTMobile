package com.twobit.gtmobile;

import android.app.Activity;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.support.v4.media.session.MediaSessionCompat;
import android.support.v4.media.session.PlaybackStateCompat;
import android.util.Log;
import android.view.inputmethod.InputMethodManager;
import android.view.KeyEvent;

import androidx.annotation.NonNull;
import androidx.core.app.ActivityCompat;
import androidx.core.app.NotificationCompat;


public class MainActivity extends Activity {

    private static final int NOTIFICATION_PERMISSION_CODE = 1;

    private static final String TAG = "Main";
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

    private MediaSessionCompat mMediaSession;
    public static final String CHANNEL_ID = "GTMobileChannel";

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

        // create notification channel
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationChannel channel = new NotificationChannel(
                    CHANNEL_ID,
                    "GTMobile Channel",
                    NotificationManager.IMPORTANCE_LOW
            );
            channel.enableVibration(false);
            channel.setSound(null, null);
            NotificationManager manager = getSystemService(NotificationManager.class);
            manager.createNotificationChannel(channel);
        }

        mMediaSession = new MediaSessionCompat(this, "GTMobile");
        mMediaSession.setCallback(new MediaSessionCompat.Callback() {
            @Override
            public void onPlay() {
                super.onPlay();
                Native.setPlaying(true, true);
                updateNotification(true);
            }
            @Override
            public void onPause() {
                super.onPause();
                Native.setPlaying(false, true);
                updateNotification(false);
            }
        });
        mMediaSession.setActive(true);
    }

    private void removeNotification() {
        NotificationManager manager = getSystemService(NotificationManager.class);
        manager.cancelAll();
    }
    private void updateNotification(boolean isPlaying) {

        int s = isPlaying ? PlaybackStateCompat.STATE_PLAYING : PlaybackStateCompat.STATE_PAUSED;
        PlaybackStateCompat playbackState = new PlaybackStateCompat.Builder()
            .setActions(PlaybackStateCompat.ACTION_PLAY_PAUSE)
            .setState(s, PlaybackStateCompat.PLAYBACK_POSITION_UNKNOWN, 1.0f)
            .build();
        mMediaSession.setPlaybackState(playbackState);

        String songName = Native.getSongName();
        if (songName.isEmpty()) songName = "<unnamed>";

        Notification notification = new NotificationCompat.Builder(this, CHANNEL_ID)
                .setSmallIcon(R.drawable.ic_launcher_foreground)
                .setContentTitle("GTMobile")
                .setContentText(songName)
                .setStyle(
                    new androidx.media.app.NotificationCompat.MediaStyle()
                    .setMediaSession(mMediaSession.getSessionToken())
                )
                .setPriority(NotificationCompat.PRIORITY_LOW)
                .setDefaults(0)
                .build();

        NotificationManager manager = getSystemService(NotificationManager.class);
        manager.notify(1, notification);
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
        removeNotification();
        Native.setPlaying(false, false);
        Native.free();
        mMediaSession.setActive(false);
        mMediaSession.release();
        super.onDestroy();
    }

    @Override
    protected void onPause() {
        Log.i(TAG, "onPause");
        super.onPause();
        mView.onPause();
        saveSettings();
        if (Native.isPlayerPlaying()) {
            updateNotification(true);
        }
        else {
            Native.setPlaying(false, false);
            updateNotification(false);
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
        removeNotification();
    }

    @Override
    public boolean onKeyDown(final int code, final KeyEvent e) {
        mView.queueEvent(new Runnable() { public void run() {
            Native.key(code, e.getUnicodeChar());
        }});
        return false;
    }
}
