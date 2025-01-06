package com.twobit.gtmobile;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Intent;
import android.os.Build;
import android.os.IBinder;
import android.support.v4.media.session.MediaSessionCompat;
import android.support.v4.media.session.PlaybackStateCompat;
import android.util.Log;

import androidx.core.app.NotificationCompat;

public class MusicService extends Service {
    static final String TAG = "Service";

    public static final String CHANNEL_ID = "GTMobileChannel";
    private MediaSessionCompat mMediaSession;

    @Override
    public void onCreate() {
        Log.i(TAG, "onCreate");
        super.onCreate();

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

        // Initialize MediaSessionCompat
        mMediaSession = new MediaSessionCompat(this, "GTMobile");
        mMediaSession.setCallback(new MediaSessionCompat.Callback() {
            @Override
            public void onPlay() {
                super.onPlay();
                updateState(true);
            }
            @Override
            public void onPause() {
                super.onPause();
                updateState(false);
            }
        });
        mMediaSession.setActive(true);
        PlaybackStateCompat playbackState = new PlaybackStateCompat.Builder()
                .setActions(PlaybackStateCompat.ACTION_PLAY | PlaybackStateCompat.ACTION_PAUSE)
                .setState(PlaybackStateCompat.STATE_NONE, 0, 1.0f)
                .build();
        mMediaSession.setPlaybackState(playbackState);
    }

    private Notification buildNotification() {
        String songName = Native.getSongName();
        if (songName.isEmpty()) songName = "<unnamed>";
        return new NotificationCompat.Builder(this, CHANNEL_ID)
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
    }

    private void setMediaSessionPlaybackState(boolean isPlaying) {
        int s = isPlaying ? PlaybackStateCompat.STATE_PLAYING : PlaybackStateCompat.STATE_PAUSED;
        PlaybackStateCompat playbackState = new PlaybackStateCompat.Builder()
            .setActions(PlaybackStateCompat.ACTION_PLAY_PAUSE)
            .setState(s, PlaybackStateCompat.PLAYBACK_POSITION_UNKNOWN, 1.0f)
            .build();
        mMediaSession.setPlaybackState(playbackState);
    }

    private void updateState(boolean isPlaying) {
        Log.i(TAG, "updateState " + isPlaying);
        Native.setPlaying(isPlaying, true);
        setMediaSessionPlaybackState(isPlaying);
        NotificationManager manager = getSystemService(NotificationManager.class);
        manager.notify(1, buildNotification());
    }
    private void removeNotification() {
        NotificationManager manager = getSystemService(NotificationManager.class);
        manager.cancelAll();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.i(TAG, "onStartCommand " + intent + " " + flags + " " + startId);
        setMediaSessionPlaybackState(Native.isPlayerPlaying());
        startForeground(1, buildNotification());
        return START_STICKY;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        mMediaSession.release();
        removeNotification();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
}