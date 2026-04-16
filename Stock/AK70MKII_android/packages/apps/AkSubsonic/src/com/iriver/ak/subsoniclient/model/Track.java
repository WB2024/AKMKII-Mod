package com.iriver.ak.subsoniclient.model;

/**
 * Represents a single audio track from the Subsonic/Navidrome library.
 * Populated from getAlbum (embedded song array) responses.
 */
public class Track {
    private String id;
    private String title;
    private String albumId;
    private String albumName;
    private String artistName;
    private String coverArtId;
    private int trackNumber;
    private int duration; // seconds
    private int discNumber;
    private String contentType; // e.g. "audio/flac"

    public Track(String id, String title, String albumId, String albumName,
                 String artistName, String coverArtId, int trackNumber,
                 int duration, int discNumber, String contentType) {
        this.id = id;
        this.title = title;
        this.albumId = albumId;
        this.albumName = albumName;
        this.artistName = artistName;
        this.coverArtId = coverArtId;
        this.trackNumber = trackNumber;
        this.duration = duration;
        this.discNumber = discNumber;
        this.contentType = contentType;
    }

    public String getId() { return id; }
    public String getTitle() { return title; }
    public String getAlbumId() { return albumId; }
    public String getAlbumName() { return albumName; }
    public String getArtistName() { return artistName; }
    public String getCoverArtId() { return coverArtId; }
    public int getTrackNumber() { return trackNumber; }
    public int getDuration() { return duration; }
    public int getDiscNumber() { return discNumber; }
    public String getContentType() { return contentType; }

    /**
     * Human-readable duration string, e.g. "3:45"
     */
    public String getDurationString() {
        int secs = duration % 60;
        int mins = duration / 60;
        return String.format("%d:%02d", mins, secs);
    }

    @Override
    public String toString() { return title; }
}
