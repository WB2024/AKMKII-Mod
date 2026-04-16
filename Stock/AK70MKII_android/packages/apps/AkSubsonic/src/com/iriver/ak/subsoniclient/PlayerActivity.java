package com.iriver.ak.subsoniclient;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.view.View;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.SeekBar;
import android.widget.TextView;

import com.iriver.ak.subsoniclient.api.SubsonicClient;
import com.iriver.ak.subsoniclient.model.Track;
import com.iriver.ak.subsoniclient.service.PlayerService;
import com.iriver.ak.subsoniclient.util.ImageLoader;
import com.iriver.ak.subsoniclient.util.PreferenceUtil;

/**
 * PlayerActivity — the "Now Playing" screen.
 *
 * Binds to PlayerService to read and control playback state.
 * Registers a BroadcastReceiver for PlayerService.ACTION_STATE_CHANGED
 * to update UI immediately on track change or play/pause toggle.
 *
 * The seek bar position polls the service every second via a Handler runnable
 * while the activity is in the foreground.
 */
public class PlayerActivity extends Activity {

    private ImageView   ivCoverArt;
    private TextView    tvTitle;
    private TextView    tvArtist;
    private TextView    tvAlbum;
    private TextView    tvElapsed;
    private TextView    tvDuration;
    private SeekBar     seekBar;
    private ImageButton btnPrevious;
    private ImageButton btnPlayPause;
    private ImageButton btnNext;

    private PlayerService playerService;
    private boolean serviceBound = false;
    private SubsonicClient client;

    private final Handler seekHandler = new Handler();
    private boolean userSeeking = false;

    // -----------------------------------------------------------------------
    // Broadcast receiver — listens for playback state changes from PlayerService
    // -----------------------------------------------------------------------

    private final BroadcastReceiver stateReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            updateUi();
        }
    };

    // -----------------------------------------------------------------------
    // Service connection
    // -----------------------------------------------------------------------

    private final ServiceConnection serviceConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, IBinder binder) {
            PlayerService.PlayerBinder pb = (PlayerService.PlayerBinder) binder;
            playerService = pb.getService();
            serviceBound  = true;
            updateUi();
            startSeekUpdater();
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            serviceBound = false;
        }
    };

    // -----------------------------------------------------------------------
    // Activity lifecycle
    // -----------------------------------------------------------------------

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_player);

        ivCoverArt   = (ImageView)   findViewById(R.id.iv_cover_art);
        tvTitle      = (TextView)    findViewById(R.id.tv_title);
        tvArtist     = (TextView)    findViewById(R.id.tv_artist);
        tvAlbum      = (TextView)    findViewById(R.id.tv_album);
        tvElapsed    = (TextView)    findViewById(R.id.tv_elapsed);
        tvDuration   = (TextView)    findViewById(R.id.tv_duration);
        seekBar      = (SeekBar)     findViewById(R.id.seek_bar);
        btnPrevious  = (ImageButton) findViewById(R.id.btn_previous);
        btnPlayPause = (ImageButton) findViewById(R.id.btn_play_pause);
        btnNext      = (ImageButton) findViewById(R.id.btn_next);

        client = new SubsonicClient(
                PreferenceUtil.getServerUrl(this),
                PreferenceUtil.getUsername(this),
                PreferenceUtil.getPassword(this)
        );

        btnPrevious.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (serviceBound) playerService.previous();
            }
        });

        btnPlayPause.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (serviceBound) playerService.playPause();
            }
        });

        btnNext.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (serviceBound) playerService.next();
            }
        });

        seekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar bar, int progress, boolean fromUser) {
                if (fromUser) {
                    tvElapsed.setText(formatTime(progress));
                }
            }

            @Override
            public void onStartTrackingTouch(SeekBar bar) {
                userSeeking = true;
            }

            @Override
            public void onStopTrackingTouch(SeekBar bar) {
                userSeeking = false;
                if (serviceBound) {
                    playerService.seekTo(bar.getProgress());
                }
            }
        });

        // Bind to service (which should already be running)
        Intent serviceIntent = new Intent(this, PlayerService.class);
        startService(serviceIntent);
        bindService(serviceIntent, serviceConnection, BIND_AUTO_CREATE);
    }

    @Override
    protected void onResume() {
        super.onResume();
        registerReceiver(stateReceiver,
                new IntentFilter(PlayerService.ACTION_STATE_CHANGED));
        if (serviceBound) {
            updateUi();
            startSeekUpdater();
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        unregisterReceiver(stateReceiver);
        stopSeekUpdater();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (serviceBound) {
            unbindService(serviceConnection);
            serviceBound = false;
        }
    }

    // -----------------------------------------------------------------------
    // UI update helpers
    // -----------------------------------------------------------------------

    private void updateUi() {
        if (!serviceBound || playerService == null) return;

        Track track = playerService.getCurrentTrack();

        if (track != null) {
            tvTitle.setText(track.getTitle());
            tvArtist.setText(track.getArtistName());
            tvAlbum.setText(track.getAlbumName());

            // Load cover art
            if (track.getCoverArtId() != null && !track.getCoverArtId().isEmpty()) {
                String artUrl = client.getCoverArtUrl(track.getCoverArtId(), 300);
                ImageLoader.getInstance().load(artUrl, ivCoverArt);
            } else {
                ivCoverArt.setImageDrawable(null);
            }

            int duration = playerService.getDuration();
            seekBar.setMax(duration > 0 ? duration : 1);
            tvDuration.setText(formatTime(duration));
        } else {
            tvTitle.setText(R.string.player_nothing_playing);
            tvArtist.setText("");
            tvAlbum.setText("");
            ivCoverArt.setImageDrawable(null);
            seekBar.setProgress(0);
            tvElapsed.setText(formatTime(0));
            tvDuration.setText(formatTime(0));
        }

        // Play/pause button icon
        if (playerService.isPlaying()) {
            btnPlayPause.setImageResource(android.R.drawable.ic_media_pause);
        } else {
            btnPlayPause.setImageResource(android.R.drawable.ic_media_play);
        }
    }

    // -----------------------------------------------------------------------
    // Seek bar polling — runs every 500 ms while activity is in foreground
    // -----------------------------------------------------------------------

    private final Runnable seekUpdater = new Runnable() {
        @Override
        public void run() {
            if (serviceBound && playerService != null && !userSeeking) {
                int pos = playerService.getCurrentPosition();
                seekBar.setProgress(pos);
                tvElapsed.setText(formatTime(pos));
            }
            seekHandler.postDelayed(this, 500);
        }
    };

    private void startSeekUpdater() {
        seekHandler.removeCallbacks(seekUpdater);
        seekHandler.post(seekUpdater);
    }

    private void stopSeekUpdater() {
        seekHandler.removeCallbacks(seekUpdater);
    }

    // -----------------------------------------------------------------------
    // Utility
    // -----------------------------------------------------------------------

    private static String formatTime(int milliseconds) {
        int totalSeconds = milliseconds / 1000;
        int secs = totalSeconds % 60;
        int mins = totalSeconds / 60;
        return String.format("%d:%02d", mins, secs);
    }
}
