package com.iriver.ak.subsoniclient.util;

import android.content.Context;
import android.content.SharedPreferences;

/**
 * Thin wrapper around SharedPreferences for server connection settings.
 * Credentials are stored in app-private storage (Android UID sandbox).
 */
public class PreferenceUtil {

    private static final String PREFS_NAME = "ak_subsonic_prefs";

    public static final String KEY_SERVER_URL  = "server_url";
    public static final String KEY_USERNAME    = "username";
    public static final String KEY_PASSWORD    = "password";
    public static final String KEY_IS_CONFIGURED = "is_configured";

    private static SharedPreferences getPrefs(Context context) {
        return context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
    }

    public static void saveServerConfig(Context context, String serverUrl,
                                        String username, String password) {
        getPrefs(context).edit()
                .putString(KEY_SERVER_URL, serverUrl)
                .putString(KEY_USERNAME, username)
                .putString(KEY_PASSWORD, password)
                .putBoolean(KEY_IS_CONFIGURED, true)
                .apply();
    }

    public static String getServerUrl(Context context) {
        return getPrefs(context).getString(KEY_SERVER_URL, "");
    }

    public static String getUsername(Context context) {
        return getPrefs(context).getString(KEY_USERNAME, "");
    }

    public static String getPassword(Context context) {
        return getPrefs(context).getString(KEY_PASSWORD, "");
    }

    public static boolean isConfigured(Context context) {
        return getPrefs(context).getBoolean(KEY_IS_CONFIGURED, false);
    }

    public static void clearConfig(Context context) {
        getPrefs(context).edit().clear().apply();
    }
}
