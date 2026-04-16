# AkSubsonic — Subsonic/Navidrome Client for AK70 MKII

## Overview

AkSubsonic is a Subsonic-protocol music streaming client built natively for the Astell&Kern AK70 MKII,
compiled into the AOSP system image using the existing Android.mk build infrastructure. It connects to a
self-hosted Navidrome (or any Subsonic-compatible) server on the local network, browses the music
library by artist → album → track, and streams audio for playback through the device's DSP chain.

---

## Device Constraints (AK70 MKII)

| Constraint | Detail |
|---|---|
| Android API level | 21 (Android 5.0 Lollipop) |
| Build system | AOSP Android.mk — no Gradle, no Maven |
| Screen density | HDPI (`LOCAL_AAPT_FLAGS += -c hdpi`) |
| Architecture | ARMv7-a + ARMv8-a (32/64-bit) |
| Certificate | `shared` (platform shared key) |
| Install path | `/system/priv-app/AkSubsonic/` (`LOCAL_PRIVILEGED_MODULE := true`) |
| Min SDK | 21 — cleartext HTTP allowed, no runtime permissions model |
| Available audio libs | `libtinyalsa`, `libsoxr`, `libffmpeg_extractor`, `libffmpeg_omx`, `libALAC` |
| Package namespace | `com.iriver.ak.*` |
| Theme | `@android:style/Theme.Material` (Material Dark — available in API 21) |

### UI Sizing Notes
- The AkIME keyboard layout uses `keypad_width = 360dp`, indicating the device's effective screen
  width is at minimum 360dp in the orientation where the keyboard appears.
- All layouts use `fill_parent` / `match_parent` to adapt to the actual device density.
- Only HDPI bitmap resources are packaged; vector drawables (no density) are used where possible.

---

## Architecture

```
com.iriver.ak.subsoniclient/
├── LoginActivity            — Server URL / credentials setup; ping test
├── LibraryActivity          — Primary screen: Artists list with tab bar
├── AlbumListActivity        — Albums for a selected artist
├── TrackListActivity        — Tracks for a selected album
├── PlayerActivity           — Now-playing screen
│
├── api/
│   └── SubsonicClient       — HTTP REST client; token-based auth (MD5); all API calls
│
├── model/
│   ├── Artist               — id, name, albumCount, coverArtId
│   ├── Album                — id, name, artist, artistId, coverArtId, year, songCount, duration
│   └── Track                — id, title, album, albumId, artist, duration, track, coverArtId
│
├── service/
│   └── PlayerService        — Foreground service; MediaPlayer; MediaSession; audio focus; queue
│
├── adapter/
│   ├── ArtistAdapter        — ListView adapter for Artist list items
│   ├── AlbumAdapter         — ListView adapter for Album list items
│   └── TrackAdapter         — ListView adapter for Track list items
│
└── util/
    ├── PreferenceUtil       — SharedPreferences wrapper (server URL, username, password)
    └── ImageLoader          — Async HTTP image loading with in-memory LRU-style cache
```

---

## Subsonic REST API

**Base URL format:**
```
http://<host>:<port>/rest/<method>.view
```

**Auth parameters (token-based, Subsonic API ≥ 1.13.0):**

| Param | Value |
|---|---|
| `u` | username |
| `t` | `MD5(password + salt)` |
| `s` | random salt (6 alphanumeric chars) |
| `v` | `1.16.1` |
| `c` | `AkSubsonic` |
| `f` | `json` |

**Endpoints implemented:**

| Method | Description |
|---|---|
| `ping.view` | Test server connectivity and credentials |
| `getArtists.view` | List all artists (ID3 tag indexed) |
| `getArtist.view?id=ID` | Get artist details + album list |
| `getAlbum.view?id=ID` | Get album details + track list |
| `stream.view?id=ID&format=raw` | Stream audio (passthrough, no transcode) |
| `getCoverArt.view?id=ID&size=N` | Get album/artist cover art image |
| `search3.view?query=Q` | Search artists, albums, songs |

**Stream URL note:** The stream URL embeds auth parameters in the query string and is passed directly
to `MediaPlayer.setDataSource()`. With `format=raw`, the original file is served without server-side
transcoding, ensuring bit-perfect playback through the device's audio chain.

---

## Build Integration

Add to the device product makefile (`device.mk` or product packages):
```makefile
PRODUCT_PACKAGES += AkSubsonic
```

The module is `LOCAL_MODULE_TAGS := optional`, so it must be explicitly listed.

### Dependencies
- No JNI libraries — pure Java using Android framework APIs
- `java.net.HttpURLConnection` for HTTP
- `org.json` (bundled with Android) for JSON parsing
- `java.security.MessageDigest` for MD5 token generation
- `android.media.MediaPlayer` for streaming playback
- `android.media.session.MediaSession` for hardware media button support (API 21)
- `android.app.Notification.MediaStyle` for lockscreen/notification controls (API 21)

---

## Reference Apps Studied

| App | URL | Key patterns used |
|---|---|---|
| DSub (daneren2005) | https://github.com/daneren2005/Subsonic | Subsonic service/REST pattern, queue management |
| Tempo (CappielloAntonio) | https://github.com/CappielloAntonio/tempo | Modern Subsonic API usage, album art loading |
| Audinaut (andrewrabert) | https://github.com/andrewrabert/Audinaut | Lightweight client, offline caching design |

Key differences from reference apps: this app uses **zero support/third-party libraries** and the
**AOSP Android.mk** build system, making it fully self-contained in the ROM.

---

## Application Flow

```
Launch
  │
  ├─ No saved server → LoginActivity
  │     └── Enter IP:port, user, pass → ping test → save → LibraryActivity
  │
  ├─ Has saved server → ping test
  │     ├── OK → LibraryActivity
  │     └── Fail → LoginActivity (with error message)
  │
LibraryActivity  [Artists | Albums | Search]
  │
  ├── Tap artist → AlbumListActivity (getArtist API)
  │     └── Tap album → TrackListActivity (getAlbum API)
  │           └── Tap track → PlayerService.play(queue, index) → PlayerActivity
  │
  └── Tap album (Albums tab) → TrackListActivity
        └── Tap track → PlayerService.play(queue, index) → PlayerActivity

PlayerActivity ←→ PlayerService (Binder)
  │   Controls: Play/Pause · Previous · Next · Seek
  └── Updates: Title · Artist · Album Art · Progress bar
```

---

## Security Considerations

1. **Password storage**: Stored in app-private SharedPreferences (sandboxed by Android UID). Not
   accessible to other apps without root. Suitable for a dedicated single-user device.
2. **Auth tokens**: MD5(password + salt) per Subsonic spec. A new salt is generated for each request,
   so captured tokens cannot be replayed.
3. **Cleartext HTTP**: Allowed on API 21 by default; appropriate for local LAN use (no external
   internet exposure). `android:usesCleartextTraffic="true"` is set explicitly for documentation.
4. **No external code**: All network code uses Android framework only — no third-party HTTP clients
   that could introduce vulnerabilities.

---

## File Manifest

### Build Files
- `Android.mk`
- `AndroidManifest.xml`

### Java Sources (`src/com/iriver/ak/subsoniclient/`)
- `LoginActivity.java`
- `LibraryActivity.java`
- `AlbumListActivity.java`
- `TrackListActivity.java`
- `PlayerActivity.java`
- `api/SubsonicClient.java`
- `model/Artist.java`
- `model/Album.java`
- `model/Track.java`
- `service/PlayerService.java`
- `adapter/ArtistAdapter.java`
- `adapter/AlbumAdapter.java`
- `adapter/TrackAdapter.java`
- `util/PreferenceUtil.java`
- `util/ImageLoader.java`

### Resources (`res/`)
- `layout/activity_login.xml`
- `layout/activity_library.xml`
- `layout/activity_album_list.xml`
- `layout/activity_track_list.xml`
- `layout/activity_player.xml`
- `layout/list_item_artist.xml`
- `layout/list_item_album.xml`
- `layout/list_item_track.xml`
- `drawable/bg_button.xml`
- `drawable/bg_button_accent.xml`
- `drawable/bg_list_item_selector.xml`
- `drawable/ic_launcher.xml`
- `values/strings.xml`
- `values/colors.xml`
- `values/dimens.xml`

---

## Changelog

### v1.0.0 (initial)
- Server login screen (IP, port, username, password)
- Token-based Subsonic auth (MD5 + salt)
- Artist browse (getArtists)
- Album browse per artist (getArtist)
- Track list per album (getAlbum)
- Audio streaming via MediaPlayer (format=raw for lossless passthrough)
- Foreground PlayerService with MediaSession
- Hardware media button support via MediaSession
- Async album art loading with in-memory cache
- Persistent server credentials via SharedPreferences
- Dark Material theme optimised for HDPI small screen
