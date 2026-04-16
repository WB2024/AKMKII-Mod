package com.iriver.ak.subsoniclient;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.iriver.ak.subsoniclient.adapter.ArtistAdapter;
import com.iriver.ak.subsoniclient.api.SubsonicClient;
import com.iriver.ak.subsoniclient.model.Artist;
import com.iriver.ak.subsoniclient.util.PreferenceUtil;

import java.util.List;

/**
 * LibraryActivity — the main screen after a successful login.
 *
 * Shows a scrollable list of all artists from the Navidrome library.
 * Tapping an artist opens AlbumListActivity for that artist.
 *
 * The ActionBar overflow menu provides:
 *   - "Now Playing"  — jump to PlayerActivity
 *   - "Settings"     — return to LoginActivity to change server
 */
public class LibraryActivity extends Activity {

    public static final String EXTRA_ARTIST_ID   = "artist_id";
    public static final String EXTRA_ARTIST_NAME = "artist_name";

    private ListView  listView;
    private ProgressBar progressBar;
    private TextView  tvEmpty;
    private SubsonicClient client;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_library);

        listView    = (ListView)    findViewById(R.id.list_artists);
        progressBar = (ProgressBar) findViewById(R.id.progress_bar);
        tvEmpty     = (TextView)    findViewById(R.id.tv_empty);

        client = new SubsonicClient(
                PreferenceUtil.getServerUrl(this),
                PreferenceUtil.getUsername(this),
                PreferenceUtil.getPassword(this)
        );

        listView.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                Artist artist = (Artist) parent.getItemAtPosition(position);
                Intent intent = new Intent(LibraryActivity.this, AlbumListActivity.class);
                intent.putExtra(EXTRA_ARTIST_ID, artist.getId());
                intent.putExtra(EXTRA_ARTIST_NAME, artist.getName());
                startActivity(intent);
            }
        });

        loadArtists();
    }

    private void loadArtists() {
        progressBar.setVisibility(View.VISIBLE);
        tvEmpty.setVisibility(View.GONE);

        client.getArtists(new SubsonicClient.Callback<List<Artist>>() {
            @Override
            public void onResult(List<Artist> artists) {
                progressBar.setVisibility(View.GONE);
                if (artists.isEmpty()) {
                    tvEmpty.setText(R.string.empty_artists);
                    tvEmpty.setVisibility(View.VISIBLE);
                } else {
                    listView.setAdapter(new ArtistAdapter(LibraryActivity.this, artists, client));
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
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.library_menu, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        int id = item.getItemId();
        if (id == R.id.menu_now_playing) {
            startActivity(new Intent(this, PlayerActivity.class));
            return true;
        } else if (id == R.id.menu_settings) {
            startActivity(new Intent(this, LoginActivity.class));
            return true;
        }
        return super.onOptionsItemSelected(item);
    }
}
