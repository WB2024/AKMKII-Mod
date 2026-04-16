package com.iriver.ak.subsoniclient.service;

import android.app.Notification;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.session.MediaSession;
import android.media.session.PlaybackState;
import android.os.Binder;
import android.os.IBinder;
import android.os.PowerManager;

import com.iriver.ak.subsoniclient.PlayerActivity;
import com.iriver.ak.subsoniclient.R;
import com.iriver.ak.subsoniclient.api.SubsonicClient;
import com.iriver.ak.subsoniclient.model.Track;

import java.util.ArrayList;
import java.util.List;

/**
 * PlayerService — background foreground service managing audio playback.
 *
 * Responsibilities:
 *   - Owns the MediaPlayer instance (streaming via HTTP stream URL)
 *   - Manages the playback queue and current track index
 *   - Handles audio focus via AudioManager
 *   - Exposes MediaSession for hardware media button support (KEYCODE_MEDIA_*)
 *   - Shows and updates a foreground notification with playback controls
 *   - Exposes a Binder so Activities can call play/pause/next/previous/seek
 *
 * Compatible with Android API 21 (Android 5.0) — no support library required.
 */
public class PlayerService extends Service
        implements MediaPlayer.OnPreparedListener,
                   MediaPlayer.OnCompletionListener,
                   MediaPlayer.OnErrorListener,
                   AudioManager.OnAudioFocusChangeListener {

    // -----------------------------------------------------------------------
    // Constants
    // -----------------------------------------------------------------------

    private static final int NOTIFICATION_ID = 1001;

    /** Broadcast sent to PlayerActivity to refresh UI state */
    public static final String ACTION_STATE_CHANGED = "com.iriver.ak.subsoniclient.STATE_CHANGED";
    public static final String EXTRA_IS_PLAYING     = "is_playing";

    // -----------------------------------------------------------------------
    // Binder
    // -----------------------------------------------------------------------

    public class PlayerBinder extends Binder {
        public PlayerService getService() {
            return PlayerService.this;
        }
    }

    private final IBinder binder = new PlayerBinder();

    // -----------------------------------------------------------------------
    // Playback state
    // -----------------------------------------------------------------------

    private MediaPlayer mediaPlayer;
    private MediaSession mediaSession;
    private AudioManager audioManager;

    private SubsonicClient client;
    private List<Track> queue = new ArrayList<>();
    private int currentIndex = -1;
    private boolean isPreparing = false;
    private boolean hasAudioFocus = false;

    // -----------------------------------------------------------------------
    // Service lifecycle
    // -----------------------------------------------------------------------

    @Override
    public void onCreate() {
        super.onCreate();
        audioManager = (AudioManager) getSystemService(AUDIO_SERVICE);
        setupMediaSession();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return binder;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        abandonAudioFocus();
        releaseMediaPlayer();
        if (mediaSession != null) {
            mediaSession.release();
            mediaSession = null;
        }
    }

    // -----------------------------------------------------------------------
    // Public interface (called by Activities via Binder)
    // -----------------------------------------------------------------------

    /**
     * Loads a new queue and starts playback at the given index.
     *
     * @param tracks  Full list of tracks (e.g. all tracks in an album)
     * @param index   Index of the track to play first
     * @param client  SubsonicClient instance for building stream URLs
     */
    public void setQueue(List<Track> tracks, int index, SubsonicClient client) {
        this.client = client;
        this.queue.clear();
        this.queue.addAll(tracks);
        this.currentIndex = index;
        playCurrentTrack();
    }

    public void play() {
        if (mediaPlayer != null && !mediaPlayer.isPlaying() && !isPreparing) {
            if (requestAudioFocus()) {
                mediaPlayer.start();
                updatePlaybackState(PlaybackState.STATE_PLAYING);
                updateNotification();
                broadcastStateChange(true);
            }
        }
    }

    public void pause() {
        if (mediaPlayer != null && mediaPlayer.isPlaying()) {
            mediaPlayer.pause();
            updatePlaybackState(PlaybackState.STATE_PAUSED);
            updateNotification();
            broadcastStateChange(false);
        }
    }

    public void playPause() {
        if (isPlaying()) {
            pause();
        } else {
            play();
        }
    }

    public void next() {
        if (queue.isEmpty()) return;
        currentIndex = (currentIndex + 1) % queue.size();
        playCurrentTrack();
    }

    public void previous() {
        if (queue.isEmpty()) return;
        // If more than 3 seconds in, restart current track; otherwise go back
        if (mediaPlayer != null && mediaPlayer.getCurrentPosition() > 3000) {
            mediaPlayer.seekTo(0);
        } else {
            currentIndex = (currentIndex - 1 + queue.size()) % queue.size();
            playCurrentTrack();
        }
    }

    public void seekTo(int positionMs) {
        if (mediaPlayer != null && !isPreparing) {
            mediaPlayer.seekTo(positionMs);
        }
    }

    public boolean isPlaying() {
        return mediaPlayer != null && !isPreparing && mediaPlayer.isPlaying();
    }

    public boolean isPreparing() {
        return isPreparing;
    }

    public Track getCurrentTrack() {
        if (currentIndex >= 0 && currentIndex < queue.size()) {
            return queue.get(currentIndex);
        }
        return null;
    }

    public int getCurrentPosition() {
        if (mediaPlayer != null && !isPreparing) {
            return mediaPlayer.getCurrentPosition();
        }
        return 0;
    }

    public int getDuration() {
        if (mediaPlayer != null && !isPreparing) {
            return mediaPlayer.getDuration();
        }
        return 0;
    }

    public int getQueueSize() { return queue.size(); }
    public int getCurrentIndex() { return currentIndex; }

    // -----------------------------------------------------------------------
    // Private: playback
    // -----------------------------------------------------------------------

    private void playCurrentTrack() {
        if (currentIndex < 0 || currentIndex >= queue.size() || client == null) return;

        Track track = queue.get(currentIndex);
        String streamUrl = client.getStreamUrl(track.getId());

        releaseMediaPlayer();
        isPreparing = true;
        broadcastStateChange(false);

        if (!requestAudioFocus()) return;

        mediaPlayer = new MediaPlayer();
        mediaPlayer.setAudioStreamType(AudioManager.STREAM_MUSIC);
        mediaPlayer.setWakeMode(getApplicationContext(), PowerManager.PARTIAL_WAKE_LOCK);
        mediaPlayer.setOnPreparedListener(this);
        mediaPlayer.setOnCompletionListener(this);
        mediaPlayer.setOnErrorListener(this);

        try {
            mediaPlayer.setDataSource(streamUrl);
            mediaPlayer.prepareAsync();
        } catch (Exception e) {
            isPreparing = false;
            broadcastStateChange(false);
        }

        showNotification();
    }

    @Override
    public void onPrepared(MediaPlayer mp) {
        isPreparing = false;
        mp.start();
        updatePlaybackState(PlaybackState.STATE_PLAYING);
        updateNotification();
        broadcastStateChange(true);
    }

    @Override
    public void onCompletion(MediaPlayer mp) {
        // Advance to next track automatically
        if (currentIndex < queue.size() - 1) {
            next();
        } else {
            // End of queue
            isPreparing = false;
            broadcastStateChange(false);
            updatePlaybackState(PlaybackState.STATE_STOPPED);
            updateNotification();
        }
    }

    @Override
    public boolean onError(MediaPlayer mp, int what, int extra) {
        isPreparing = false;
        broadcastStateChange(false);
        return true; // Handled
    }

    private void releaseMediaPlayer() {
        if (mediaPlayer != null) {
            if (mediaPlayer.isPlaying()) mediaPlayer.stop();
            mediaPlayer.reset();
            mediaPlayer.release();
            mediaPlayer = null;
        }
        isPreparing = false;
    }

    // -----------------------------------------------------------------------
    // Private: audio focus
    // -----------------------------------------------------------------------

    private boolean requestAudioFocus() {
        if (hasAudioFocus) return true;
        int result = audioManager.requestAudioFocus(
                this, AudioManager.STREAM_MUSIC, AudioManager.AUDIOFOCUS_GAIN);
        hasAudioFocus = (result == AudioManager.AUDIOFOCUS_REQUEST_GRANTED);
        return hasAudioFocus;
    }

    private void abandonAudioFocus() {
        audioManager.abandonAudioFocus(this);
        hasAudioFocus = false;
    }

    @Override
    public void onAudioFocusChange(int focusChange) {
        switch (focusChange) {
            case AudioManager.AUDIOFOCUS_GAIN:
                if (mediaPlayer != null) {
                    mediaPlayer.setVolume(1.0f, 1.0f);
                    play();
                }
                break;
            case AudioManager.AUDIOFOCUS_LOSS:
                pause();
                abandonAudioFocus();
                break;
            case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT:
                pause();
                break;
            case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK:
                if (mediaPlayer != null) mediaPlayer.setVolume(0.2f, 0.2f);
                break;
        }
    }

    // -----------------------------------------------------------------------
    // Private: MediaSession (hardware media button support)
    // -----------------------------------------------------------------------

    private void setupMediaSession() {
        mediaSession = new MediaSession(this, "AkSubsonicSession");
        mediaSession.setFlags(
                MediaSession.FLAG_HANDLES_MEDIA_BUTTONS |
                MediaSession.FLAG_HANDLES_TRANSPORT_CONTROLS);

        mediaSession.setCallback(new MediaSession.Callback() {
            @Override
            public void onPlay()     { play(); }
            @Override
            public void onPause()    { pause(); }
            @Override
            public void onSkipToNext()     { next(); }
            @Override
            public void onSkipToPrevious() { previous(); }
            @Override
            public void onStop() {
                pause();
                stopForeground(true);
                stopSelf();
            }
            @Override
            public void onSeekTo(long pos) { seekTo((int) pos); }
        });

        mediaSession.setActive(true);
        updatePlaybackState(PlaybackState.STATE_NONE);
    }

    private void updatePlaybackState(int state) {
        if (mediaSession == null) return;
        long actions = PlaybackState.ACTION_PLAY_PAUSE
                | PlaybackState.ACTION_SKIP_TO_NEXT
                | PlaybackState.ACTION_SKIP_TO_PREVIOUS
                | PlaybackState.ACTION_SEEK_TO
                | PlaybackState.ACTION_STOP;

        PlaybackState.Builder builder = new PlaybackState.Builder()
                .setActions(actions)
                .setState(state,
                        mediaPlayer != null && !isPreparing
                                ? mediaPlayer.getCurrentPosition() : 0,
                        1.0f);
        mediaSession.setPlaybackState(builder.build());
    }

    // -----------------------------------------------------------------------
    // Private: foreground notification
    // -----------------------------------------------------------------------

    private void showNotification() {
        startForeground(NOTIFICATION_ID, buildNotification());
    }

    private void updateNotification() {
        Notification n = buildNotification();
        android.app.NotificationManager nm =
                (android.app.NotificationManager) getSystemService(NOTIFICATION_SERVICE);
        nm.notify(NOTIFICATION_ID, n);
    }

    private Notification buildNotification() {
        Track track = getCurrentTrack();
        String title  = track != null ? track.getTitle()     : getString(R.string.app_name);
        String artist = track != null ? track.getArtistName(): "";

        Intent tapIntent = new Intent(this, PlayerActivity.class);
        tapIntent.setFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
        PendingIntent contentIntent = PendingIntent.getActivity(
                this, 0, tapIntent, PendingIntent.FLAG_UPDATE_CURRENT);

        // Prev action
        Intent prevIntent = new Intent(this, PlayerService.class);
        prevIntent.setAction("PREV");
        PendingIntent prevPending = PendingIntent.getService(
                this, 1, prevIntent, PendingIntent.FLAG_UPDATE_CURRENT);

        // Play/Pause action
        Intent ppIntent = new Intent(this, PlayerService.class);
        ppIntent.setAction("PLAY_PAUSE");
        PendingIntent ppPending = PendingIntent.getService(
                this, 2, ppIntent, PendingIntent.FLAG_UPDATE_CURRENT);

        // Next action
        Intent nextIntent = new Intent(this, PlayerService.class);
        nextIntent.setAction("NEXT");
        PendingIntent nextPending = PendingIntent.getService(
                this, 3, nextIntent, PendingIntent.FLAG_UPDATE_CURRENT);

        int playPauseIcon = isPlaying()
                ? android.R.drawable.ic_media_pause
                : android.R.drawable.ic_media_play;

        Notification.Builder builder = new Notification.Builder(this)
                .setSmallIcon(R.drawable.ic_launcher)
                .setContentTitle(title)
                .setContentText(artist)
                .setContentIntent(contentIntent)
                .setOngoing(true)
                .addAction(android.R.drawable.ic_media_previous,
                        getString(R.string.action_previous), prevPending)
                .addAction(playPauseIcon,
                        isPlaying() ? getString(R.string.action_pause) : getString(R.string.action_play),
                        ppPending)
                .addAction(android.R.drawable.ic_media_next,
                        getString(R.string.action_next), nextPending)
                .setStyle(new Notification.MediaStyle()
                        .setMediaSession(mediaSession.getSessionToken())
                        .setShowActionsInCompactView(0, 1, 2));

        return builder.build();
    }

    // -----------------------------------------------------------------------
    // Private: intent actions from notification buttons
    // -----------------------------------------------------------------------

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (intent != null && intent.getAction() != null) {
            switch (intent.getAction()) {
                case "PLAY_PAUSE": playPause(); break;
                case "NEXT":       next();      break;
                case "PREV":       previous();  break;
            }
        }
        return START_STICKY;
    }

    // -----------------------------------------------------------------------
    // Private: broadcast state to PlayerActivity
    // -----------------------------------------------------------------------

    private void broadcastStateChange(boolean isPlaying) {
        Intent intent = new Intent(ACTION_STATE_CHANGED);
        intent.putExtra(EXTRA_IS_PLAYING, isPlaying);
        sendBroadcast(intent);
    }
}
