package com.iriver.ak.subsoniclient;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.iriver.ak.subsoniclient.adapter.AlbumAdapter;
import com.iriver.ak.subsoniclient.api.SubsonicClient;
import com.iriver.ak.subsoniclient.model.Album;
import com.iriver.ak.subsoniclient.util.PreferenceUtil;

import java.util.List;

/**
 * AlbumListActivity — shows all albums for a selected artist.
 *
 * Receives EXTRA_ARTIST_ID and EXTRA_ARTIST_NAME from LibraryActivity.
 * Tapping an album navigates to TrackListActivity.
 */
public class AlbumListActivity extends Activity {

    public static final String EXTRA_ALBUM_ID   = "album_id";
    public static final String EXTRA_ALBUM_NAME = "album_name";

    private ListView    listView;
    private ProgressBar progressBar;
    private TextView    tvEmpty;
    private SubsonicClient client;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_album_list);

        String artistId   = getIntent().getStringExtra(LibraryActivity.EXTRA_ARTIST_ID);
        String artistName = getIntent().getStringExtra(LibraryActivity.EXTRA_ARTIST_NAME);

        if (getActionBar() != null) {
            getActionBar().setDisplayHomeAsUpEnabled(true);
            getActionBar().setTitle(artistName != null ? artistName : getString(R.string.albums_title));
        }

        listView    = (ListView)    findViewById(R.id.list_albums);
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
                Album album = (Album) parent.getItemAtPosition(position);
                Intent intent = new Intent(AlbumListActivity.this, TrackListActivity.class);
                intent.putExtra(EXTRA_ALBUM_ID, album.getId());
                intent.putExtra(EXTRA_ALBUM_NAME, album.getName());
                startActivity(intent);
            }
        });

        loadAlbums(artistId);
    }

    private void loadAlbums(String artistId) {
        progressBar.setVisibility(View.VISIBLE);
        tvEmpty.setVisibility(View.GONE);

        client.getArtist(artistId, new SubsonicClient.Callback<List<Album>>() {
            @Override
            public void onResult(List<Album> albums) {
                progressBar.setVisibility(View.GONE);
                if (albums.isEmpty()) {
                    tvEmpty.setText(R.string.empty_albums);
                    tvEmpty.setVisibility(View.VISIBLE);
                } else {
                    listView.setAdapter(new AlbumAdapter(AlbumListActivity.this, albums, client));
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
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == android.R.id.home) {
            onBackPressed();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }
}
