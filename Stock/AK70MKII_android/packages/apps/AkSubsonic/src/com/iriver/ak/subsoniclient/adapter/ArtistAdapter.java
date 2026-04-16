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
import com.iriver.ak.subsoniclient.model.Artist;
import com.iriver.ak.subsoniclient.util.ImageLoader;

import java.util.List;

/**
 * ListView adapter for the Artists list in LibraryActivity.
 * Each row shows a thumbnail (cover art) and the artist name + album count.
 */
public class ArtistAdapter extends ArrayAdapter<Artist> {

    private final SubsonicClient client;
    private final LayoutInflater inflater;

    public ArtistAdapter(Context context, List<Artist> artists, SubsonicClient client) {
        super(context, 0, artists);
        this.client   = client;
        this.inflater = LayoutInflater.from(context);
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        ViewHolder holder;

        if (convertView == null) {
            convertView = inflater.inflate(R.layout.list_item_artist, parent, false);
            holder = new ViewHolder();
            holder.ivArt       = (ImageView) convertView.findViewById(R.id.iv_art);
            holder.tvName      = (TextView)  convertView.findViewById(R.id.tv_name);
            holder.tvSubtitle  = (TextView)  convertView.findViewById(R.id.tv_subtitle);
            convertView.setTag(holder);
        } else {
            holder = (ViewHolder) convertView.getTag();
        }

        Artist artist = getItem(position);
        holder.tvName.setText(artist.getName());

        int albumCount = artist.getAlbumCount();
        holder.tvSubtitle.setText(getContext().getResources().getQuantityString(
                R.plurals.album_count, albumCount, albumCount));

        // Load cover art asynchronously
        String coverArtId = artist.getCoverArtId();
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
