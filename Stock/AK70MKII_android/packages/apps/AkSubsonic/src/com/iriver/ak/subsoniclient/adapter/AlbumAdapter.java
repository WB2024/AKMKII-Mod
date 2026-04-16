package com.iriver.ak.subsoniclient.adapter;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.TextView;

import com.iriver.ak.subsoniclient.R;
import com.iriver.ak.subsoniclient.api.SubsonicClient;
import com.iriver.ak.subsoniclient.model.Album;
import com.iriver.ak.subsoniclient.util.ImageLoader;

import java.util.List;

/**
 * ListView adapter for the Albums list in AlbumListActivity.
 * Each row shows the album cover art, album name, year, and track count.
 */
public class AlbumAdapter extends ArrayAdapter<Album> {

    private final SubsonicClient client;
    private final LayoutInflater inflater;

    public AlbumAdapter(Context context, List<Album> albums, SubsonicClient client) {
        super(context, 0, albums);
        this.client   = client;
        this.inflater = LayoutInflater.from(context);
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        ViewHolder holder;

        if (convertView == null) {
            convertView = inflater.inflate(R.layout.list_item_album, parent, false);
            holder = new ViewHolder();
            holder.ivArt      = (ImageView) convertView.findViewById(R.id.iv_art);
            holder.tvName     = (TextView)  convertView.findViewById(R.id.tv_name);
            holder.tvSubtitle = (TextView)  convertView.findViewById(R.id.tv_subtitle);
            convertView.setTag(holder);
        } else {
            holder = (ViewHolder) convertView.getTag();
        }

        Album album = getItem(position);
        holder.tvName.setText(album.getName());

        // Subtitle: "Year · N tracks"
        StringBuilder subtitle = new StringBuilder();
        if (album.getYear() > 0) {
            subtitle.append(album.getYear()).append("  ·  ");
        }
        int songs = album.getSongCount();
        subtitle.append(getContext().getResources().getQuantityString(
                R.plurals.track_count, songs, songs));
        holder.tvSubtitle.setText(subtitle.toString());

        String coverArtId = album.getCoverArtId();
        if (client != null && coverArtId != null && !coverArtId.isEmpty()) {
            String url = client.getCoverArtUrl(coverArtId, 128);
            ImageLoader.getInstance().load(url, holder.ivArt);
        } else {
            holder.ivArt.setImageResource(R.drawable.ic_launcher);
        }

        return convertView;
    }

    private static class ViewHolder {
        ImageView ivArt;
        TextView  tvName;
        TextView  tvSubtitle;
    }
}
