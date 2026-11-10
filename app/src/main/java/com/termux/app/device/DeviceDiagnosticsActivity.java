package com.termux.app.device;

import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.core.content.ContextCompat;
import androidx.core.widget.TextViewCompat;

import com.termux.R;
import com.termux.shared.activity.ActivityUtils;
import com.termux.shared.logger.Logger;

import java.text.DecimalFormat;
import java.util.Locale;

public class DeviceDiagnosticsActivity extends AppCompatActivity {

    private DeviceDiagnosticsManager mDiagnosticsManager;
    private ScrollView mScrollView;
    private LinearLayout mContentLayout;
    private View mProgressBar;
    private TextView mErrorView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_device_diagnostics);

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        if (getSupportActionBar() != null) {
            getSupportActionBar().setDisplayHomeAsUpEnabled(true);
            getSupportActionBar().setTitle(R.string.title_device);
        }

        mDiagnosticsManager = new DeviceDiagnosticsManager(this);

        mScrollView = findViewById(R.id.scroll_view);
        mContentLayout = findViewById(R.id.content_layout);
        mProgressBar = findViewById(R.id.progress_bar);
        mErrorView = findViewById(R.id.error_view);

        refreshDiagnostics();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.menu_device_diagnostics, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        int id = item.getItemId();
        if (id == android.R.id.home) {
            finish();
            return true;
        } else if (id == R.id.action_refresh) {
            refreshDiagnostics();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    private void refreshDiagnostics() {
        mProgressBar.setVisibility(View.VISIBLE);
        mContentLayout.setVisibility(View.GONE);
        mErrorView.setVisibility(View.GONE);

        new Thread(() -> {
            try {
                final DeviceDiagnosticsManager.DeviceInfo info = mDiagnosticsManager.getDeviceInfo();
                runOnUiThread(() -> {
                    mProgressBar.setVisibility(View.GONE);
                    mContentLayout.setVisibility(View.VISIBLE);
                    populateViews(info);
                });
            } catch (Exception e) {
                Logger.logStackTraceWithMessage(DeviceDiagnosticsManager.LOG_TAG, "Failed to get device info", e);
                runOnUiThread(() -> {
                    mProgressBar.setVisibility(View.GONE);
                    mErrorView.setVisibility(View.VISIBLE);
                    mErrorView.setText(getString(R.string.msg_permission_denied) + ": " + e.getMessage());
                    Toast.makeText(this, R.string.msg_permission_denied, Toast.LENGTH_SHORT).show();
                });
            }
        }).start();
    }

    private void populateViews(DeviceDiagnosticsManager.DeviceInfo info) {
        mContentLayout.removeAllViews();

        addSection(R.string.device_cpu, buildCpuView(info.cpu));
        addSection(R.string.device_memory, buildMemoryView(info.memory));
        addSection(R.string.device_storage, buildStorageView(info.storage));
        addSection(R.string.device_battery, buildBatteryView(info.battery));
        addSection(R.string.device_thermal, buildThermalView(info.thermal));
        addSection(R.string.device_network, buildNetworkView(info.network));
        addSection(R.string.device_android_version, buildSystemView(info.system));
    }

    private void addSection(int titleResId, View contentView) {
        TextView title = new TextView(this);
        title.setText(titleResId);
        TextViewCompat.setTextAppearance(title, android.R.style.TextAppearance_Medium);
        title.setPadding(16, 24, 16, 8);
        mContentLayout.addView(title);
        mContentLayout.addView(contentView);
    }

    private View buildCpuView(DeviceDiagnosticsManager.CpuInfo cpu) {
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setPadding(16, 0, 16, 16);

        addInfoRow(layout, getString(R.string.device_architecture), cpu.architecture);
        addInfoRow(layout, getString(R.string.device_model), cpu.model);
        addInfoRow(layout, getString(R.string.device_cores), String.valueOf(cpu.cores));

        if (cpu.currentFrequencies != null && !cpu.currentFrequencies.isEmpty()) {
            StringBuilder freqStr = new StringBuilder();
            for (int i = 0; i < cpu.currentFrequencies.size(); i++) {
                if (i > 0) freqStr.append(", ");
                freqStr.append(formatFrequency(cpu.currentFrequencies.get(i)));
                if (cpu.maxFrequencies != null && i < cpu.maxFrequencies.size()) {
                    freqStr.append(" / ").append(formatFrequency(cpu.maxFrequencies.get(i)));
                }
            }
            addInfoRow(layout, getString(R.string.device_frequency), freqStr.toString());
        }

        return layout;
    }

    private View buildMemoryView(DeviceDiagnosticsManager.MemoryInfo mem) {
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setPadding(16, 0, 16, 16);

        addInfoRow(layout, getString(R.string.device_total_memory), formatBytes(mem.totalMemory));
        addInfoRow(layout, getString(R.string.device_available_memory), formatBytes(mem.availableMemory));
        addInfoRow(layout, "Low memory", mem.lowMemory ? "Yes" : "No");
        addInfoRow(layout, "Threshold", formatBytes(mem.threshold));

        return layout;
    }

    private View buildStorageView(DeviceDiagnosticsManager.StorageInfo storage) {
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setPadding(16, 0, 16, 16);

        addInfoRow(layout, getString(R.string.device_total_storage), formatBytes(storage.totalStorage));
        addInfoRow(layout, getString(R.string.device_free_storage), formatBytes(storage.freeStorage));
        addInfoRow(layout, "Usable", formatBytes(storage.usableStorage));
        if (storage.externalTotalStorage > 0) {
            addInfoRow(layout, "External total", formatBytes(storage.externalTotalStorage));
            addInfoRow(layout, "External free", formatBytes(storage.externalFreeStorage));
        }

        return layout;
    }

    private View buildBatteryView(DeviceDiagnosticsManager.BatteryInfo battery) {
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setPadding(16, 0, 16, 16);

        addInfoRow(layout, getString(R.string.device_battery_level), battery.level + "%");
        addInfoRow(layout, getString(R.string.device_battery_status), battery.status);
        addInfoRow(layout, getString(R.string.device_battery_health), battery.health);
        addInfoRow(layout, getString(R.string.device_battery_technology), battery.technology);
        addInfoRow(layout, getString(R.string.device_battery_temperature), String.format(Locale.getDefault(), "%.1f°C", battery.temperature));
        addInfoRow(layout, getString(R.string.device_battery_voltage), String.format(Locale.getDefault(), "%.2fV", battery.voltage));
        if (battery.currentNow != 0) {
            addInfoRow(layout, "Current", String.format(Locale.getDefault(), "%.2fA", battery.currentNow));
        }

        return layout;
    }

    private View buildThermalView(DeviceDiagnosticsManager.ThermalInfo thermal) {
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setPadding(16, 0, 16, 16);

        addInfoRow(layout, getString(R.string.device_thermal_status), thermal.status != null ? thermal.status : getString(R.string.msg_not_available));

        if (thermal.zones != null && !thermal.zones.isEmpty()) {
            for (DeviceDiagnosticsManager.ThermalZone zone : thermal.zones) {
                String zoneInfo = String.format(Locale.getDefault(), "%s: %.1f°C (%s)",
                    zone.name != null ? zone.name : "Unknown",
                    zone.temperature,
                    zone.mode != null ? zone.mode : "N/A");
                addInfoRow(layout, "Zone", zoneInfo);
            }
        }

        return layout;
    }

    private View buildNetworkView(DeviceDiagnosticsManager.NetworkInfo net) {
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setPadding(16, 0, 16, 16);

        addInfoRow(layout, getString(R.string.device_network_type), net.type != null ? net.type : getString(R.string.msg_not_available));
        addInfoRow(layout, "Metered", net.isMetered ? "Yes" : "No");
        addInfoRow(layout, "VPN", net.isVPN ? "Yes" : "No");

        if (net.wifiSsid != null && !net.wifiSsid.isEmpty() && !net.wifiSsid.equals("<unknown ssid>")) {
            addInfoRow(layout, getString(R.string.device_wifi_ssid), net.wifiSsid);
            addInfoRow(layout, "BSSID", net.wifiBssid);
            addInfoRow(layout, getString(R.string.device_wifi_signal), net.wifiSignalStrength + "/5");
            addInfoRow(layout, "RSSI", net.wifiRssi + " dBm");
            addInfoRow(layout, "Link speed", net.wifiLinkSpeed + " Mbps");
            addInfoRow(layout, "Frequency", net.wifiFrequency + " MHz");
        }

        if (net.mobileNetworkOperator != null && !net.mobileNetworkOperator.isEmpty()) {
            addInfoRow(layout, getString(R.string.device_mobile_network), net.mobileNetworkOperator);
            addInfoRow(layout, "Type", net.mobileNetworkType);
            addInfoRow(layout, "Signal", net.mobileSignalStrength >= 0 ? net.mobileSignalStrength + "/4" : getString(R.string.msg_not_available));
        }

        if (net.ipAddresses != null && !net.ipAddresses.isEmpty()) {
            for (String ip : net.ipAddresses) {
                addInfoRow(layout, getString(R.string.device_ip_address), ip);
            }
        }

        return layout;
    }

    private View buildSystemView(DeviceDiagnosticsManager.SystemInfo sys) {
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setPadding(16, 0, 16, 16);

        addInfoRow(layout, getString(R.string.device_kernel_version), sys.kernelVersion != null ? sys.kernelVersion.split(" ")[0] : getString(R.string.msg_not_available));
        addInfoRow(layout, getString(R.string.device_android_version), sys.androidVersion);
        addInfoRow(layout, getString(R.string.device_api_level), String.valueOf(sys.apiLevel));
        addInfoRow(layout, getString(R.string.device_security_patch), sys.securityPatch);
        addInfoRow(layout, getString(R.string.device_build_number), sys.buildNumber);
        addInfoRow(layout, getString(R.string.device_manufacturer), sys.manufacturer);
        addInfoRow(layout, getString(R.string.device_model), sys.model);
        addInfoRow(layout, getString(R.string.device_fingerprint), sys.fingerprint);
        addInfoRow(layout, "Hardware", sys.hardware);
        addInfoRow(layout, "Device", sys.device);
        addInfoRow(layout, "Product", sys.product);
        addInfoRow(layout, "Brand", sys.brand);
        addInfoRow(layout, getString(R.string.device_uptime), sys.uptime);

        return layout;
    }

    private void addInfoRow(LinearLayout parent, String label, String value) {
        LinearLayout row = new LinearLayout(this);
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setPadding(0, 8, 0, 8);

        TextView labelView = new TextView(this);
        labelView.setText(label);
        labelView.setLayoutParams(new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f));
        labelView.setTextColor(ContextCompat.getColor(this, android.R.color.secondary_text_dark));

        TextView valueView = new TextView(this);
        valueView.setText(value != null ? value : getString(R.string.msg_not_available));
        valueView.setLayoutParams(new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f));
        valueView.setTextAlignment(View.TEXT_ALIGNMENT_VIEW_END);
        valueView.setTextColor(ContextCompat.getColor(this, android.R.color.primary_text_dark));

        row.addView(labelView);
        row.addView(valueView);
        parent.addView(row);
    }

    private String formatBytes(long bytes) {
        if (bytes <= 0) return "0 B";
        String[] units = {"B", "KB", "MB", "GB", "TB"};
        int unitIndex = (int) (Math.log(bytes) / Math.log(1024));
        if (unitIndex >= units.length) unitIndex = units.length - 1;
        double value = bytes / Math.pow(1024, unitIndex);
        return new DecimalFormat("#.##").format(value) + " " + units[unitIndex];
    }

    private String formatFrequency(long hz) {
        if (hz >= 1_000_000_000L) {
            return new DecimalFormat("#.##").format(hz / 1_000_000_000.0) + " GHz";
        } else if (hz >= 1_000_000L) {
            return new DecimalFormat("#.##").format(hz / 1_000_000.0) + " MHz";
        } else {
            return new DecimalFormat("#.##").format(hz / 1_000.0) + " kHz";
        }
    }
}
