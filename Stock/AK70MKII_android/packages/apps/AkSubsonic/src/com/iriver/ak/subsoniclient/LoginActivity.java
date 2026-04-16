package com.iriver.ak.subsoniclient;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.iriver.ak.subsoniclient.api.SubsonicClient;
import com.iriver.ak.subsoniclient.util.PreferenceUtil;

/**
 * LoginActivity — first screen shown when the server is not yet configured.
 *
 * Allows the user to enter:
 *   - Server IP address and port (e.g. 192.168.1.10:4533)
 *   - Username and password
 *
 * Performs a Subsonic "ping" to verify the credentials before saving.
 * On success, stores settings in SharedPreferences and launches LibraryActivity.
 */
public class LoginActivity extends Activity {

    private EditText etHost;
    private EditText etPort;
    private EditText etUsername;
    private EditText etPassword;
    private Button   btnConnect;
    private ProgressBar progressBar;
    private TextView tvError;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_login);

        etHost      = (EditText)    findViewById(R.id.et_host);
        etPort      = (EditText)    findViewById(R.id.et_port);
        etUsername  = (EditText)    findViewById(R.id.et_username);
        etPassword  = (EditText)    findViewById(R.id.et_password);
        btnConnect  = (Button)      findViewById(R.id.btn_connect);
        progressBar = (ProgressBar) findViewById(R.id.progress_bar);
        tvError     = (TextView)    findViewById(R.id.tv_error);

        // Pre-fill if already configured (re-editing settings)
        if (PreferenceUtil.isConfigured(this)) {
            String savedUrl = PreferenceUtil.getServerUrl(this);
            // Separate the host:port back out for display
            String stripped = savedUrl.replace("http://", "").replace("https://", "");
            String[] parts = stripped.split(":");
            if (parts.length == 2) {
                etHost.setText(parts[0]);
                etPort.setText(parts[1]);
            } else {
                etHost.setText(stripped);
            }
            etUsername.setText(PreferenceUtil.getUsername(this));
            etPassword.setText(PreferenceUtil.getPassword(this));
        }

        btnConnect.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                attemptConnect();
            }
        });

        // If already successfully configured, skip straight to library
        if (PreferenceUtil.isConfigured(this)) {
            pingAndProceed(
                    PreferenceUtil.getServerUrl(this),
                    PreferenceUtil.getUsername(this),
                    PreferenceUtil.getPassword(this),
                    false
            );
        }
    }

    private void attemptConnect() {
        tvError.setVisibility(View.GONE);

        String host     = etHost.getText().toString().trim();
        String portStr  = etPort.getText().toString().trim();
        String username = etUsername.getText().toString().trim();
        String password = etPassword.getText().toString();

        if (TextUtils.isEmpty(host)) {
            showError(getString(R.string.error_host_required));
            return;
        }
        if (TextUtils.isEmpty(username)) {
            showError(getString(R.string.error_username_required));
            return;
        }
        if (TextUtils.isEmpty(password)) {
            showError(getString(R.string.error_password_required));
            return;
        }

        // Build the server URL
        String port = TextUtils.isEmpty(portStr) ? "4533" : portStr;
        String serverUrl = "http://" + host + ":" + port;

        pingAndProceed(serverUrl, username, password, true);
    }

    private void pingAndProceed(final String serverUrl, final String username,
                                final String password, final boolean saveOnSuccess) {
        setLoading(true);

        SubsonicClient client = new SubsonicClient(serverUrl, username, password);
        client.ping(new SubsonicClient.Callback<Boolean>() {
            @Override
            public void onResult(Boolean result) {
                setLoading(false);
                if (saveOnSuccess) {
                    PreferenceUtil.saveServerConfig(
                            LoginActivity.this, serverUrl, username, password);
                }
                startActivity(new Intent(LoginActivity.this, LibraryActivity.class));
                finish();
            }

            @Override
            public void onError(String errorMessage) {
                setLoading(false);
                showError(getString(R.string.error_connection_failed) + "\n" + errorMessage);
            }
        });
    }

    private void setLoading(boolean loading) {
        progressBar.setVisibility(loading ? View.VISIBLE : View.GONE);
        btnConnect.setEnabled(!loading);
    }

    private void showError(String message) {
        tvError.setText(message);
        tvError.setVisibility(View.VISIBLE);
    }
}
