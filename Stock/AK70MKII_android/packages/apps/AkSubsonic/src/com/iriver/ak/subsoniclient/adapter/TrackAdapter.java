package com.iriver.ak.subsoniclient.adapter;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.TextView;

import com.iriver.ak.subsoniclient.R;
import com.iriver.ak.subsoniclient.model.Track;

import java.util.List;

/**
 * ListView adapter for the Tracks list in TrackListActivity.
 * Each row shows the track number, title, artist, and duration.
 * No cover art per row (all tracks in an album share the same art).
 */
public class TrackAdapter extends ArrayAdapter<Track> {

    private final LayoutInflater inflater;

    public TrackAdapter(Context context, List<Track> tracks) {
        super(context, 0, tracks);
        this.inflater = LayoutInflater.from(context);
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        ViewHolder holder;

        if (convertView == null) {
            convertView = inflater.inflate(R.layout.list_item_track, parent, false);
            holder = new ViewHolder();
            holder.tvTrackNum = (TextView) convertView.findViewById(R.id.tv_track_num);
            holder.tvTitle    = (TextView) convertView.findViewById(R.id.tv_title);
            holder.tvArtist   = (TextView) convertView.findViewById(R.id.tv_artist);
            holder.tvDuration = (TextView) convertView.findViewById(R.id.tv_duration);
            convertView.setTag(holder);
        } else {
            holder = (ViewHolder) convertView.getTag();
        }

        Track track = getItem(position);

        int trackNum = track.getTrackNumber();
        holder.tvTrackNum.setText(trackNum > 0 ? String.valueOf(trackNum) : "·");
        holder.tvTitle.setText(track.getTitle());
        holder.tvArtist.setText(track.getArtistName());
        holder.tvDuration.setText(track.getDurationString());

        return convertView;
    }

    private static class ViewHolder {
        TextView tvTrackNum;
        TextView tvTitle;
        TextView tvArtist;
        TextView tvDuration;
    }
}
