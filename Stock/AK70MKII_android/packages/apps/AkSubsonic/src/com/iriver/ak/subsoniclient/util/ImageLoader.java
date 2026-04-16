package com.iriver.ak.subsoniclient.util;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Handler;
import android.os.Looper;
import android.widget.ImageView;

import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.Map;

/**
 * Simple asynchronous image loader backed by a fixed-size in-memory LRU cache.
 * Does not require any support library — uses only Android framework APIs.
 *
 * Usage:
 *   ImageLoader.getInstance().load(coverArtUrl, imageView);
 */
public class ImageLoader {

    private static final int MAX_CACHE_SIZE = 30;

    private static ImageLoader instance;

    // LinkedHashMap in access-order mode acts as an LRU cache
    private final Map<String, Bitmap> cache = Collections.synchronizedMap(
            new LinkedHashMap<String, Bitmap>(MAX_CACHE_SIZE, 0.75f, true) {
                @Override
                protected boolean removeEldestEntry(Map.Entry<String, Bitmap> eldest) {
                    return size() > MAX_CACHE_SIZE;
                }
            });

    private final Handler mainHandler = new Handler(Looper.getMainLooper());

    private ImageLoader() {}

    public static ImageLoader getInstance() {
        if (instance == null) {
            instance = new ImageLoader();
        }
        return instance;
    }

    /**
     * Loads an image from {@code url} into {@code target} asynchronously.
     * Uses the cache if available. Clears the ImageView immediately so stale
     * images don't show while the new one is loading.
     */
    public void load(final String url, final ImageView target) {
        if (url == null || url.isEmpty()) {
            target.setImageDrawable(null);
            return;
        }

        // Tag the view so we can discard late-arriving results
        target.setTag(url);
        target.setImageDrawable(null);

        Bitmap cached = cache.get(url);
        if (cached != null) {
            target.setImageBitmap(cached);
            return;
        }

        new Thread(new Runnable() {
            @Override
            public void run() {
                final Bitmap bmp = downloadBitmap(url);
                if (bmp != null) {
                    cache.put(url, bmp);
                    mainHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            // Only apply if the view still wants this URL
                            if (url.equals(target.getTag())) {
                                target.setImageBitmap(bmp);
                            }
                        }
                    });
                }
            }
        }).start();
    }

    private Bitmap downloadBitmap(String urlStr) {
        HttpURLConnection conn = null;
        try {
            URL url = new URL(urlStr);
            conn = (HttpURLConnection) url.openConnection();
            conn.setConnectTimeout(8000);
            conn.setReadTimeout(15000);
            conn.setDoInput(true);
            conn.connect();
            if (conn.getResponseCode() == 200) {
                InputStream is = conn.getInputStream();
                return BitmapFactory.decodeStream(is);
            }
        } catch (Exception e) {
            // Silently ignore — image just won't load
        } finally {
            if (conn != null) conn.disconnect();
        }
        return null;
    }

    public void clearCache() {
        cache.clear();
    }
}
