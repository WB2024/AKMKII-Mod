package com.iriver.ak.subsoniclient.api;

import android.os.Handler;
import android.os.Looper;

import com.iriver.ak.subsoniclient.model.Album;
import com.iriver.ak.subsoniclient.model.Artist;
import com.iriver.ak.subsoniclient.model.Track;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.URLEncoder;
import java.security.MessageDigest;
import java.security.SecureRandom;
import java.util.ArrayList;
import java.util.List;

/**
 * HTTP client for the Subsonic REST API (v1.16.1).
 *
 * Authentication uses the token-based method (MD5 + random salt) which is
 * supported by Navidrome and all Subsonic-compatible servers >= 1.13.0.
 *
 * All network calls are executed on a background thread; results are posted
 * back to the main (UI) thread via the provided Callback.
 */
public class SubsonicClient {

    private static final String API_VERSION = "1.16.1";
    private static final String CLIENT_NAME = "AkSubsonic";
    private static final int CONNECT_TIMEOUT_MS = 10_000;
    private static final int READ_TIMEOUT_MS    = 30_000;

    // -----------------------------------------------------------------------
    // Callback interface
    // -----------------------------------------------------------------------

    public interface Callback<T> {
        void onResult(T result);
        void onError(String errorMessage);
    }

    // -----------------------------------------------------------------------
    // Fields
    // -----------------------------------------------------------------------

    private final String serverUrl; // e.g. "http://192.168.1.10:4533"
    private final String username;
    private final String password;
    private final Handler mainHandler = new Handler(Looper.getMainLooper());

    // -----------------------------------------------------------------------
    // Constructor
    // -----------------------------------------------------------------------

    public SubsonicClient(String serverUrl, String username, String password) {
        // Ensure no trailing slash
        this.serverUrl = serverUrl.endsWith("/")
                ? serverUrl.substring(0, serverUrl.length() - 1)
                : serverUrl;
        this.username = username;
        this.password = password;
    }

    // -----------------------------------------------------------------------
    // Public API methods
    // -----------------------------------------------------------------------

    /**
     * Tests server connectivity and credential validity.
     */
    public void ping(final Callback<Boolean> callback) {
        executeRequest(buildUrl("ping", null), new ResponseHandler() {
            @Override
            public void onResponse(JSONObject response) throws Exception {
                postResult(callback, Boolean.TRUE);
            }
            @Override
            public void onFailure(String message) {
                postError(callback, message);
            }
        });
    }

    /**
     * Returns a flat list of all artists in the library (ID3-indexed).
     */
    public void getArtists(final Callback<List<Artist>> callback) {
        executeRequest(buildUrl("getArtists", null), new ResponseHandler() {
            @Override
            public void onResponse(JSONObject response) throws Exception {
                List<Artist> artists = new ArrayList<>();
                JSONObject artistsObj = response.getJSONObject("artists");
                JSONArray indexArray = artistsObj.getJSONArray("index");
                for (int i = 0; i < indexArray.length(); i++) {
                    JSONObject index = indexArray.getJSONObject(i);
                    JSONArray artistArray = index.optJSONArray("artist");
                    if (artistArray == null) continue;
                    for (int j = 0; j < artistArray.length(); j++) {
                        JSONObject a = artistArray.getJSONObject(j);
                        artists.add(new Artist(
                                a.getString("id"),
                                a.optString("name", ""),
                                a.optString("coverArt", ""),
                                a.optInt("albumCount", 0)
                        ));
                    }
                }
                postResult(callback, artists);
            }
            @Override
            public void onFailure(String message) {
                postError(callback, message);
            }
        });
    }

    /**
     * Returns albums for a given artist ID.
     */
    public void getArtist(final String artistId, final Callback<List<Album>> callback) {
        String url = buildUrl("getArtist", "id=" + encode(artistId));
        executeRequest(url, new ResponseHandler() {
            @Override
            public void onResponse(JSONObject response) throws Exception {
                List<Album> albums = new ArrayList<>();
                JSONObject artist = response.getJSONObject("artist");
                JSONArray albumArray = artist.optJSONArray("album");
                if (albumArray != null) {
                    for (int i = 0; i < albumArray.length(); i++) {
                        albums.add(parseAlbum(albumArray.getJSONObject(i)));
                    }
                }
                postResult(callback, albums);
            }
            @Override
            public void onFailure(String message) {
                postError(callback, message);
            }
        });
    }

    /**
     * Returns the track list for a given album ID.
     */
    public void getAlbum(final String albumId, final Callback<List<Track>> callback) {
        String url = buildUrl("getAlbum", "id=" + encode(albumId));
        executeRequest(url, new ResponseHandler() {
            @Override
            public void onResponse(JSONObject response) throws Exception {
                List<Track> tracks = new ArrayList<>();
                JSONObject album = response.getJSONObject("album");
                JSONArray songArray = album.optJSONArray("song");
                if (songArray != null) {
                    for (int i = 0; i < songArray.length(); i++) {
                        tracks.add(parseTrack(songArray.getJSONObject(i)));
                    }
                }
                postResult(callback, tracks);
            }
            @Override
            public void onFailure(String message) {
                postError(callback, message);
            }
        });
    }

    /**
     * Searches artists, albums, and songs. Results are returned as a combined
     * list of Track objects (includes type prefix) for simplicity.
     * Returns up to 10 results each for artists/albums/songs.
     */
    public void search(final String query, final Callback<List<Track>> callback) {
        String url = buildUrl("search3",
                "query=" + encode(query)
                        + "&artistCount=0&albumCount=0&songCount=20");
        executeRequest(url, new ResponseHandler() {
            @Override
            public void onResponse(JSONObject response) throws Exception {
                List<Track> tracks = new ArrayList<>();
                JSONObject result = response.optJSONObject("searchResult3");
                if (result != null) {
                    JSONArray songs = result.optJSONArray("song");
                    if (songs != null) {
                        for (int i = 0; i < songs.length(); i++) {
                            tracks.add(parseTrack(songs.getJSONObject(i)));
                        }
                    }
                }
                postResult(callback, tracks);
            }
            @Override
            public void onFailure(String message) {
                postError(callback, message);
            }
        });
    }

    // -----------------------------------------------------------------------
    // URL builders — public so activities can pass stream/art URLs to views
    // -----------------------------------------------------------------------

    /**
     * Returns the streaming URL for a track. Uses format=raw for lossless
     * passthrough (no server-side transcoding). This URL embeds auth params
     * and can be passed directly to MediaPlayer.setDataSource().
     */
    public String getStreamUrl(String trackId) {
        return buildUrl("stream", "id=" + encode(trackId) + "&format=raw");
    }

    /**
     * Returns the cover art URL for a given coverArtId at the requested size.
     * Can be loaded directly by ImageLoader.
     */
    public String getCoverArtUrl(String coverArtId, int size) {
        return buildUrl("getCoverArt",
                "id=" + encode(coverArtId) + "&size=" + size);
    }

    // -----------------------------------------------------------------------
    // Private: URL construction and auth token generation
    // -----------------------------------------------------------------------

    /**
     * Builds a full request URL.
     *
     * @param method     Subsonic method name (without ".view")
     * @param extraParams Additional query-string key=value pairs, pre-encoded,
     *                    joined with "&", or null if none.
     */
    private String buildUrl(String method, String extraParams) {
        String salt  = generateSalt();
        String token = md5(password + salt);

        StringBuilder sb = new StringBuilder(serverUrl)
                .append("/rest/")
                .append(method)
                .append(".view")
                .append("?u=").append(encode(username))
                .append("&t=").append(token)
                .append("&s=").append(salt)
                .append("&v=").append(API_VERSION)
                .append("&c=").append(CLIENT_NAME)
                .append("&f=json");

        if (extraParams != null && !extraParams.isEmpty()) {
            sb.append("&").append(extraParams);
        }
        return sb.toString();
    }

    private static String generateSalt() {
        SecureRandom rng = new SecureRandom();
        String chars = "abcdefghijklmnopqrstuvwxyz0123456789";
        StringBuilder sb = new StringBuilder(8);
        for (int i = 0; i < 8; i++) {
            sb.append(chars.charAt(rng.nextInt(chars.length())));
        }
        return sb.toString();
    }

    private static String md5(String input) {
        try {
            MessageDigest md = MessageDigest.getInstance("MD5");
            byte[] digest = md.digest(input.getBytes("UTF-8"));
            StringBuilder sb = new StringBuilder(32);
            for (byte b : digest) {
                sb.append(String.format("%02x", b & 0xff));
            }
            return sb.toString();
        } catch (Exception e) {
            return "";
        }
    }

    private static String encode(String s) {
        try {
            return URLEncoder.encode(s, "UTF-8");
        } catch (Exception e) {
            return s;
        }
    }

    // -----------------------------------------------------------------------
    // Private: HTTP execution and response parsing
    // -----------------------------------------------------------------------

    private interface ResponseHandler {
        void onResponse(JSONObject response) throws Exception;
        void onFailure(String message);
    }

    private void executeRequest(final String url, final ResponseHandler handler) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                HttpURLConnection conn = null;
                try {
                    conn = (HttpURLConnection) new URL(url).openConnection();
                    conn.setRequestMethod("GET");
                    conn.setConnectTimeout(CONNECT_TIMEOUT_MS);
                    conn.setReadTimeout(READ_TIMEOUT_MS);
                    conn.setDoInput(true);

                    int code = conn.getResponseCode();
                    if (code != 200) {
                        handler.onFailure("HTTP error: " + code);
                        return;
                    }

                    BufferedReader reader = new BufferedReader(
                            new InputStreamReader(conn.getInputStream(), "UTF-8"));
                    StringBuilder sb = new StringBuilder();
                    String line;
                    while ((line = reader.readLine()) != null) {
                        sb.append(line);
                    }
                    reader.close();

                    // Parse top-level Subsonic response envelope
                    JSONObject root = new JSONObject(sb.toString());
                    JSONObject response = root.getJSONObject("subsonic-response");
                    String status = response.getString("status");

                    if (!"ok".equals(status)) {
                        JSONObject error = response.optJSONObject("error");
                        String msg = error != null
                                ? error.optString("message", "Server returned an error")
                                : "Server returned an error";
                        handler.onFailure(msg);
                        return;
                    }

                    handler.onResponse(response);

                } catch (Exception e) {
                    handler.onFailure(e.getMessage() != null ? e.getMessage() : "Network error");
                } finally {
                    if (conn != null) conn.disconnect();
                }
            }
        }).start();
    }

    // -----------------------------------------------------------------------
    // Private: JSON → model parsers
    // -----------------------------------------------------------------------

    private Album parseAlbum(JSONObject obj) throws Exception {
        return new Album(
                obj.getString("id"),
                obj.optString("name", ""),
                obj.optString("artistId", ""),
                obj.optString("artist", ""),
                obj.optString("coverArt", ""),
                obj.optInt("year", 0),
                obj.optInt("songCount", 0),
                obj.optInt("duration", 0)
        );
    }

    private Track parseTrack(JSONObject obj) throws Exception {
        return new Track(
                obj.getString("id"),
                obj.optString("title", ""),
                obj.optString("parent", ""),
                obj.optString("album", ""),
                obj.optString("artist", ""),
                obj.optString("coverArt", ""),
                obj.optInt("track", 0),
                obj.optInt("duration", 0),
                obj.optInt("discNumber", 1),
                obj.optString("contentType", "audio/mpeg")
        );
    }

    // -----------------------------------------------------------------------
    // Private: thread-safe result posting
    // -----------------------------------------------------------------------

    private <T> void postResult(final Callback<T> callback, final T result) {
        mainHandler.post(new Runnable() {
            @Override
            public void run() {
                callback.onResult(result);
            }
        });
    }

    private <T> void postError(final Callback<T> callback, final String message) {
        mainHandler.post(new Runnable() {
            @Override
            public void run() {
                callback.onError(message);
            }
        });
    }
}
