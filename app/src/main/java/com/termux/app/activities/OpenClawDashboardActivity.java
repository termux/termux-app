package com.termux.app.activities;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.Spinner;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;
import androidx.preference.PreferenceManager;

import com.termux.R;
import com.termux.app.OpenClawService;
import com.termux.shared.logger.Logger;
import com.termux.shared.termux.TermuxConstants;
import com.termux.shared.termux.shell.command.runner.terminal.TermuxSession;

public class OpenClawDashboardActivity extends AppCompatActivity implements ServiceConnection {

    private static final String LOG_TAG = "OpenClawDashboard";
    private OpenClawService mOpenClawService;
    private WebView mWebView;
    private TextView mOpenClawStatus;
    private TextView mOllamaStatus;
    private Button mToggleOpenClawButton;
    private Button mRunPhi4Button;
    private Button mOpenClawTerminalButton;
    private Button mOllamaTerminalButton;
    private Spinner mModelSpinner;

    private final Handler mHandler = new Handler();
    private final Runnable mUpdateRunnable = new Runnable() {
        @Override
        public void run() {
            updateStatus();
            mHandler.postDelayed(this, 2000);
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_open_claw_dashboard);

        mWebView = findViewById(R.id.open_claw_webview);
        mOpenClawStatus = findViewById(R.id.open_claw_status);
        mOllamaStatus = findViewById(R.id.ollama_status);
        mToggleOpenClawButton = findViewById(R.id.btn_toggle_open_claw);
        mRunPhi4Button = findViewById(R.id.btn_run_phi4);
        mOpenClawTerminalButton = findViewById(R.id.btn_open_claw_terminal);
        mOllamaTerminalButton = findViewById(R.id.btn_ollama_terminal);
        mModelSpinner = findViewById(R.id.ollama_model_spinner);
        findViewById(R.id.btn_refresh_webview).setOnClickListener(v -> mWebView.reload());
        findViewById(R.id.btn_wake_lock).setOnClickListener(v -> toggleWakeLock());

        findViewById(R.id.btn_install_open_claw).setOnClickListener(v -> installOpenClaw());
        findViewById(R.id.btn_install_ollama).setOnClickListener(v -> installOllama());

        mToggleOpenClawButton.setOnClickListener(v -> toggleOpenClaw());
        mRunPhi4Button.setOnClickListener(v -> runOllama());
        mOpenClawTerminalButton.setOnClickListener(v -> openTerminal("OpenClaw"));
        mOllamaTerminalButton.setOnClickListener(v -> openTerminal("Ollama"));

        setupModelSpinner();
        setupWebView();

        Intent serviceIntent = new Intent(this, OpenClawService.class);
        bindService(serviceIntent, this, Context.BIND_AUTO_CREATE);
    }

    private void setupWebView() {
        WebSettings webSettings = mWebView.getSettings();
        webSettings.setJavaScriptEnabled(true);
        webSettings.setDomStorageEnabled(true);
        mWebView.setWebViewClient(new WebViewClient() {
            @Override
            public void onReceivedError(WebView view, int errorCode, String description, String failingUrl) {
                String title = getString(R.string.msg_gateway_not_reachable_title);
                String body = getString(R.string.msg_gateway_not_reachable_body);
                String html = "<html><body><div style='padding:20px; text-align:center;'><h3>" + title + "</h3><p>" + body + "</p></div></body></html>";
                view.loadData(html, "text/html", "UTF-8");
            }
        });
        mWebView.loadUrl("http://localhost:18789");
    }

    @Override
    public void onServiceConnected(ComponentName name, IBinder service) {
        mOpenClawService = ((OpenClawService.LocalBinder) service).service;
        mHandler.post(mUpdateRunnable);
    }

    @Override
    public void onServiceDisconnected(ComponentName name) {
        mOpenClawService = null;
    }

    private void toggleWakeLock() {
        if (mOpenClawService == null) return;
        Intent intent = new Intent(this, OpenClawService.class);
        intent.setAction(TermuxConstants.TERMUX_APP.TERMUX_SERVICE.ACTION_WAKE_LOCK);
        startService(intent);
    }

    private void setupModelSpinner() {
        ArrayAdapter<CharSequence> adapter = ArrayAdapter.createFromResource(this,
                R.array.ollama_models, android.R.layout.simple_spinner_item);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mModelSpinner.setAdapter(adapter);
    }

    private void updateStatus() {
        if (mOpenClawService == null) return;

        boolean openClawRunning = mOpenClawService.getTermuxSessionForShellName("OpenClaw") != null;
        boolean ollamaRunning = mOpenClawService.getTermuxSessionForShellName("Ollama") != null;

        mOpenClawStatus.setText(openClawRunning ? R.string.open_claw_status_running : R.string.open_claw_status_stopped);
        mToggleOpenClawButton.setText(openClawRunning ? R.string.action_stop_open_claw : R.string.action_start_open_claw);
        mOpenClawTerminalButton.setEnabled(openClawRunning);

        mOllamaStatus.setText(ollamaRunning ? R.string.ollama_status_running : R.string.ollama_status_stopped);
        mOllamaTerminalButton.setEnabled(ollamaRunning);
        mRunPhi4Button.setEnabled(!ollamaRunning);

        if (openClawRunning && mWebView.getUrl() == null) {
            mWebView.reload();
        }
    }

    private void openTerminal(String sessionName) {
        if (mOpenClawService != null) {
            mOpenClawService.switchToTermuxSession(sessionName);
        }
    }

    private void installOpenClaw() {
        if (mOpenClawService == null) return;
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);
        String cmd = prefs.getString("open_claw_install_cmd", "pkg install -y nodejs && npm install -g openclaw@latest");
        mOpenClawService.createTermuxSession(null, new String[]{"-c", cmd}, null, null, false, "Install-OpenClaw");
        mOpenClawService.switchToTermuxSession("Install-OpenClaw");
    }

    private void installOllama() {
        if (mOpenClawService == null) return;
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);
        String cmd = prefs.getString("ollama_install_cmd", "curl -L https://ollama.com/download/ollama-linux-arm64 -o $PREFIX/bin/ollama && chmod +x $PREFIX/bin/ollama");
        mOpenClawService.createTermuxSession(null, new String[]{"-c", cmd}, null, null, false, "Install-Ollama");
        mOpenClawService.switchToTermuxSession("Install-Ollama");
    }

    private void toggleOpenClaw() {
        if (mOpenClawService == null) return;

        TermuxSession session = mOpenClawService.getTermuxSessionForShellName("OpenClaw");
        if (session != null) {
            mOpenClawService.removeTermuxSession(session.getTerminalSession());
        } else {
            SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);
            String port = prefs.getString("open_claw_port", "18789");
            String cmd = "openclaw gateway --port " + port;
            mOpenClawService.createTermuxSession(null, new String[]{"-c", cmd}, null, null, false, "OpenClaw");
        }
        updateStatus();
    }

    private void runOllama() {
        if (mOpenClawService == null) return;

        String selectedModel = (String) mModelSpinner.getSelectedItem();
        if (mOpenClawService.getTermuxSessionForShellName("Ollama") == null) {
            SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);
            boolean autoDownload = prefs.getBoolean("ollama_auto_download", true);
            String cmdTemplate = prefs.getString("ollama_custom_command", "ollama run %s");
            String cmd = String.format(cmdTemplate, selectedModel);

            if (autoDownload) {
                Logger.showToast(this, "Ensuring model " + selectedModel + " is available...", true);
            }

            mOpenClawService.createTermuxSession(null, new String[]{"-c", cmd}, null, null, false, "Ollama");
        }
        updateStatus();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        mHandler.removeCallbacks(mUpdateRunnable);
        if (mOpenClawService != null) {
            unbindService(this);
        }
    }
}
