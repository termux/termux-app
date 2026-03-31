package com.termux.app.activities;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
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

import com.termux.R;
import com.termux.app.TermuxService;
import com.termux.shared.logger.Logger;
import com.termux.shared.termux.TermuxConstants;
import com.termux.shared.termux.shell.command.runner.terminal.TermuxSession;

public class OpenClawDashboardActivity extends AppCompatActivity implements ServiceConnection {

    private static final String LOG_TAG = "OpenClawDashboard";
    private TermuxService mTermuxService;
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

        mToggleOpenClawButton.setOnClickListener(v -> toggleOpenClaw());
        mRunPhi4Button.setOnClickListener(v -> runOllama());
        mOpenClawTerminalButton.setOnClickListener(v -> openTerminal("OpenClaw"));
        mOllamaTerminalButton.setOnClickListener(v -> openTerminal("Ollama"));

        setupModelSpinner();
        setupWebView();

        Intent serviceIntent = new Intent(this, TermuxService.class);
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
        mTermuxService = ((TermuxService.LocalBinder) service).service;
        mHandler.post(mUpdateRunnable);
    }

    @Override
    public void onServiceDisconnected(ComponentName name) {
        mTermuxService = null;
    }

    private void setupModelSpinner() {
        String[] models = {"phi4-mini", "llama3.2", "mistral", "gemma2"};
        ArrayAdapter<String> adapter = new ArrayAdapter<>(this, android.R.layout.simple_spinner_item, models);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mModelSpinner.setAdapter(adapter);
    }

    private void updateStatus() {
        if (mTermuxService == null) return;

        boolean openClawRunning = mTermuxService.getTermuxSessionForShellName("OpenClaw") != null;
        boolean ollamaRunning = mTermuxService.getTermuxSessionForShellName("Ollama") != null;

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
        if (mTermuxService != null) {
            mTermuxService.switchToTermuxSession(sessionName);
        }
    }

    private void toggleOpenClaw() {
        if (mTermuxService == null) return;

        TermuxSession session = mTermuxService.getTermuxSessionForShellName("OpenClaw");
        if (session != null) {
            mTermuxService.removeTermuxSession(session.getTerminalSession());
        } else {
            // Check if installed, if not install once, otherwise just run
            String cmd = "command -v openclaw >/dev/null 2>&1 || npm install -g openclaw@latest; openclaw gateway";
            mTermuxService.createTermuxSession(null, new String[]{"-c", cmd}, null, null, false, "OpenClaw");
        }
        updateStatus();
    }

    private void runOllama() {
        if (mTermuxService == null) return;

        String selectedModel = (String) mModelSpinner.getSelectedItem();
        if (mTermuxService.getTermuxSessionForShellName("Ollama") == null) {
            String cmd = "ollama run " + selectedModel;
            mTermuxService.createTermuxSession(null, new String[]{"-c", cmd}, null, null, false, "Ollama");
        }
        updateStatus();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        mHandler.removeCallbacks(mUpdateRunnable);
        if (mTermuxService != null) {
            unbindService(this);
        }
    }
}
