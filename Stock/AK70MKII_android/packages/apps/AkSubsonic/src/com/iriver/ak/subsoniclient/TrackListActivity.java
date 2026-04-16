package com.iriver.ak.subsoniclient;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.iriver.ak.subsoniclient.adapter.TrackAdapter;
import com.iriver.ak.subsoniclient.api.SubsonicClient;
import com.iriver.ak.subsoniclient.model.Track;
import com.iriver.ak.subsoniclient.service.PlayerService;
import com.iriver.ak.subsoniclient.util.PreferenceUtil;

import java.util.List;

/**
 * TrackListActivity — shows the track listing for a selected album.
 *
 * Receives EXTRA_ALBUM_ID and EXTRA_ALBUM_NAME from AlbumListActivity.
 * Tapping a track loads the full album queue into PlayerService, starts
 * playback from the chosen track, and navigates to PlayerActivity.
 */
public class TrackListActivity extends Activity {

    private ListView    listView;
    private ProgressBar progressBar;
    private TextView    tvEmpty;
    private SubsonicClient client;

    private PlayerService playerService;
    private boolean serviceBound = false;

    private List<Track> tracks;

    private final ServiceConnection serviceConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, IBinder binder) {
            PlayerService.PlayerBinder pb = (PlayerService.PlayerBinder) binder;
            playerService = pb.getService();
            serviceBound  = true;
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            serviceBound = false;
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_track_list);

        String albumId   = getIntent().getStringExtra(AlbumListActivity.EXTRA_ALBUM_ID);
        String albumName = getIntent().getStringExtra(AlbumListActivity.EXTRA_ALBUM_NAME);

        if (getActionBar() != null) {
            getActionBar().setDisplayHomeAsUpEnabled(true);
            getActionBar().setTitle(albumName != null ? albumName : getString(R.string.tracks_title));
        }

        listView    = (ListView)    findViewById(R.id.list_tracks);
        progressBar = (ProgressBar) findViewById(R.id.progress_bar);
        tvEmpty     = (TextView)    findViewById(R.id.tv_empty);

        client = new SubsonicClient(
                PreferenceUtil.getServerUrl(this),
                PreferenceUtil.getUsername(this),
                PreferenceUtil.getPassword(this)
        );

        // Bind to PlayerService so we can send the queue when a track is tapped
        Intent serviceIntent = new Intent(this, PlayerService.class);
        startService(serviceIntent);
        bindService(serviceIntent, serviceConnection, BIND_AUTO_CREATE);

        listView.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                if (tracks != null && serviceBound) {
                    playerService.setQueue(tracks, position, client);
                    startActivity(new Intent(TrackListActivity.this, PlayerActivity.class));
                }
            }
        });

        loadTracks(albumId);
    }

    private void loadTracks(String albumId) {
        progressBar.setVisibility(View.VISIBLE);
        tvEmpty.setVisibility(View.GONE);

        client.getAlbum(albumId, new SubsonicClient.Callback<List<Track>>() {
            @Override
            public void onResult(List<Track> result) {
                progressBar.setVisibility(View.GONE);
                tracks = result;
                if (result.isEmpty()) {
                    tvEmpty.setText(R.string.empty_tracks);
                    tvEmpty.setVisibility(View.VISIBLE);
                } else {
                    listView.setAdapter(new TrackAdapter(TrackListActivity.this, result));
                }
            }

            @Override
            public void onError(String errorMessage) {
                progressBar.setVisibility(View.GONE);
                tvEmpty.setText(getString(R.string.error_load_failed) + "\n" + errorMessage);
                tvEmpty.setVisibility(View.VISIBLE);
            }
        });
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (serviceBound) {
            unbindService(serviceConnection);
            serviceBound = false;
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == android.R.id.home) {
            onBackPressed();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }
}
