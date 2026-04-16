package com.iriver.ak.subsoniclient.model;

/**
 * Represents a music album from the Subsonic/Navidrome library.
 * Populated from getArtist (embedded) and getAlbum responses.
 */
public class Album {
    private String id;
    private String name;
    private String artistId;
    private String artistName;
    private String coverArtId;
    private int year;
    private int songCount;
    private int duration; // seconds

    public Album(String id, String name, String artistId, String artistName,
                 String coverArtId, int year, int songCount, int duration) {
        this.id = id;
        this.name = name;
        this.artistId = artistId;
        this.artistName = artistName;
        this.coverArtId = coverArtId;
        this.year = year;
        this.songCount = songCount;
        this.duration = duration;
    }

    public String getId() { return id; }
    public String getName() { return name; }
    public String getArtistId() { return artistId; }
    public String getArtistName() { return artistName; }
    public String getCoverArtId() { return coverArtId; }
    public int getYear() { return year; }
    public int getSongCount() { return songCount; }
    public int getDuration() { return duration; }

    public String getYearString() {
        return year > 0 ? String.valueOf(year) : "";
    }

    @Override
    public String toString() { return name; }
}
