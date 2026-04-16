package com.iriver.ak.subsoniclient.model;

/**
 * Represents a music artist from the Subsonic/Navidrome library.
 * Populated from the getArtists and getArtist API responses.
 */
public class Artist {
    private String id;
    private String name;
    private String coverArtId;
    private int albumCount;

    public Artist(String id, String name, String coverArtId, int albumCount) {
        this.id = id;
        this.name = name;
        this.coverArtId = coverArtId;
        this.albumCount = albumCount;
    }

    public String getId() { return id; }
    public String getName() { return name; }
    public String getCoverArtId() { return coverArtId; }
    public int getAlbumCount() { return albumCount; }

    @Override
    public String toString() { return name; }
}
