package org.skynet.runtime;

import android.os.Bundle;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.webkit.WebChromeClient;
import android.webkit.JavascriptInterface;
import android.util.Log;
import androidx.appcompat.app.AppCompatActivity;
import android.widget.Toast;
import java.io.File;
import java.io.FileWriter;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

public class MainActivity extends AppCompatActivity {

    private WebView webView;
    private static final String TAG = "SkyNet";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        webView = new WebView(this);
        setContentView(webView);

        // WebView config
        webView.getSettings().setJavaScriptEnabled(true);
        webView.getSettings().setDomStorageEnabled(true);
        webView.getSettings().setAllowFileAccess(true);
        webView.getSettings().setAllowContentAccess(true);
        webView.getSettings().setDatabaseEnabled(true);
        webView.getSettings().setCacheMode(android.webkit.WebSettings.LOAD_DEFAULT);

        // Enable remote debugging
        WebView.setWebContentsDebuggingEnabled(true);

        // Add JavaScript bridge
        webView.addJavascriptInterface(new SkyNetBridge(), "SkyNetBridge");

        // Load the dashboard
        webView.loadUrl("file:///android_asset/index.html");
    }

    // ── JavaScript Bridge for logging and system info ──
    public class SkyNetBridge {
        @JavascriptInterface
        public String getPlatform() {
            return "Android";
        }

        @JavascriptInterface
        public String getVersion() {
            return "1.0.0";
        }

        @JavascriptInterface
        public String getDeviceInfo() {
            return String.format(
                "{\"model\":\"%s\",\"manufacturer\":\"%s\",\"androidVersion\":\"%s\",\"apiLevel\":%d}",
                android.os.Build.MODEL,
                android.os.Build.MANUFACTURER,
                android.os.Build.VERSION.RELEASE,
                android.os.Build.VERSION.SDK_INT
            );
        }

        @JavascriptInterface
        public void log(String level, String message) {
            String logLine = String.format("[%s] %s: %s",
                new SimpleDateFormat("HH:mm:ss", Locale.getDefault()).format(new Date()),
                level, message);

            Log.d(TAG, logLine);

            // Write to app-specific log file
            try {
                File extDir = getExternalFilesDir(null);
                if (extDir == null) {
                    // Fallback: write to internal cache
                    extDir = getCacheDir();
                }
                File logDir = new File(extDir, "logs");
                if (!logDir.exists()) logDir.mkdirs();
                File logFile = new File(logDir, "skynet.log");
                FileWriter fw = new FileWriter(logFile, true);
                fw.write(logLine + "\n");
                fw.close();
            } catch (Exception e) {
                Log.e(TAG, "Failed to write log", e);
            }
        }

        @JavascriptInterface
        public String getLogs() {
            try {
                File logFile = new File(getExternalFilesDir(null), "logs/skynet.log");
                if (!logFile.exists()) return "";
                BufferedReader br = new BufferedReader(new InputStreamReader(
                    new java.io.FileInputStream(logFile)));
                StringBuilder sb = new StringBuilder();
                String line;
                while ((line = br.readLine()) != null) {
                    sb.append(line).append("\n");
                }
                br.close();
                return sb.toString();
            } catch (Exception e) {
                return "Error reading logs: " + e.getMessage();
            }
        }

        @JavascriptInterface
        public void showToast(String message) {
            runOnUiThread(() -> Toast.makeText(MainActivity.this, message, Toast.LENGTH_SHORT).show());
        }
    }

    @Override
    public void onBackPressed() {
        if (webView.canGoBack()) {
            webView.goBack();
        } else {
            super.onBackPressed();
        }
    }
}
