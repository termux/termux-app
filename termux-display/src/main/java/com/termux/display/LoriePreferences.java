package com.termux.display;

import static android.Manifest.permission.POST_NOTIFICATIONS;
import static android.Manifest.permission.WRITE_SECURE_SETTINGS;
import static android.content.pm.PackageManager.PERMISSION_DENIED;
import static android.content.pm.PackageManager.PERMISSION_GRANTED;
import static android.os.Build.VERSION.SDK_INT;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.content.res.XmlResourceParser;
import android.graphics.Typeface;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.preference.Preference;

import android.os.Handler;
import android.os.IBinder;
import android.preference.PreferenceManager;
import android.provider.Settings;

import androidx.annotation.Nullable;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.preference.PreferenceFragmentCompat;
import androidx.preference.PreferenceGroup;

import androidx.preference.Preference.OnPreferenceChangeListener;
import androidx.preference.SeekBarPreference;

import android.text.method.LinkMovementMethod;
import android.util.Log;
import android.view.MenuItem;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import com.termux.display.controller.InputControllerActivity;
import com.termux.display.controller.container.Container;
import com.termux.display.controller.container.Shortcut;
import com.termux.display.controller.contentdialog.ContentDialog;
import com.termux.display.controller.core.Callback;
import com.termux.display.controller.core.CursorLocker;
import com.termux.display.controller.core.DownloadProgressDialog;
import com.termux.display.controller.inputcontrols.ControlsProfile;
import com.termux.display.controller.inputcontrols.InputControlsManager;
import com.termux.display.controller.widget.InputControlsView;
import com.termux.display.controller.winhandler.WinHandler;
import com.termux.display.utils.KeyInterceptor;
import com.termux.display.utils.SamsungDexUtils;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;


import java.io.IOException;
import java.io.StringWriter;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Map;
import java.util.Objects;
import java.util.regex.PatternSyntaxException;

@SuppressWarnings("deprecation")
public class LoriePreferences extends AppCompatActivity {
    public static int OPEN_FILE_REQUEST_CODE = 102;
    static final String ACTION_PREFERENCES_CHANGED = "com.termux.display.ACTION_PREFERENCES_CHANGED";
    static final String SHOW_IME_WITH_HARD_KEYBOARD = "show_ime_with_hard_keyboard";
    protected LoriePreferenceFragment loriePreferenceFragment;
    protected LorieView xServer;
    protected boolean touchShow = false;
    protected int orientation = ActivityInfo.SCREEN_ORIENTATION_PORTRAIT;

    protected interface TermuxActivityListener {
        void onX11PreferenceSwitchChange(boolean isOpen);

        void releaseSlider(boolean open);

        void onChangeOrientation(int landscape);

        void reInstallX11StartScript(Activity activity);

        void stopDesktop(Activity activity);

        void openSoftwareKeyboard();
    }

    protected TermuxActivityListener termuxActivityListener;

    private final BroadcastReceiver receiver = new BroadcastReceiver() {
        @SuppressLint("UnspecifiedRegisterReceiverFlag")
        @Override
        public void onReceive(Context context, Intent intent) {
            if (ACTION_PREFERENCES_CHANGED.equals(intent.getAction())) {
                if (intent.getBooleanExtra("fromBroadcast", false)) {
                    loriePreferenceFragment.getPreferenceScreen().removeAll();
                    loriePreferenceFragment.addPreferencesFromResource(R.xml.preferences);
                }
            }
        }
    };

    //input controller
    protected InputControlsManager inputControlsManager;
    protected InputControlsView inputControlsView;
    //    protected TouchpadView touchpadView;
    protected Runnable editInputControlsCallback;
    protected Shortcut shortcut;
    protected DownloadProgressDialog preloaderDialog;
    protected Callback<Uri> openFileCallback;
    protected float globalCursorSpeed = 1.0f;
    protected ControlsProfile profile;
    protected String controlsProfile;
    protected Container container;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        loriePreferenceFragment = new LoriePreferenceFragment();
//        getSupportFragmentManager().beginTransaction().replace(android.R.id.content, loriePreferenceFragment).commit();

        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            actionBar.setDisplayHomeAsUpEnabled(true);
            actionBar.setHomeButtonEnabled(true);
            actionBar.setTitle("Preferences");
        }
        loriePreferenceFragment.setActivity(this);
    }

    @SuppressLint("WrongConstant")
    @Override
    protected void onResume() {
        super.onResume();
        IntentFilter filter = new IntentFilter(ACTION_PREFERENCES_CHANGED);
        registerReceiver(receiver, filter, SDK_INT >= Build.VERSION_CODES.TIRAMISU ? RECEIVER_NOT_EXPORTED : 0);
    }

    @Override
    protected void onPause() {
        super.onPause();
        unregisterReceiver(receiver);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        int id = item.getItemId();

        if (id == android.R.id.home) {
            finish();
            return true;
        }

        return super.onOptionsItemSelected(item);
    }

    public void installX11ServerBridge() {
        if (termuxActivityListener != null) {
            termuxActivityListener.reInstallX11StartScript(this);
        }
    }

    public void stopDesktop() {
        if (termuxActivityListener != null) {
            termuxActivityListener.stopDesktop(this);
        }
    }

    public static class LoriePreferenceFragment extends PreferenceFragmentCompat implements OnPreferenceChangeListener, Preference.OnPreferenceClickListener {
        private LoriePreferences activity;

        public void setActivity(LoriePreferences activity) {
            this.activity = activity;
        }

        @Override
        public void onCreatePreferences(@Nullable Bundle savedInstanceState, @Nullable String rootKey) {
            SharedPreferences p = getPreferenceManager().getSharedPreferences();
            int modeValue = p == null ? 0 : Integer.parseInt(p.getString("touchMode", "1")) - 1;
            if (modeValue > 2) {
                SharedPreferences.Editor e = Objects.requireNonNull(p).edit();
                e.putString("touchMode", "1");
                e.apply();
            }

            addPreferencesFromResource(R.xml.preferences);
        }

        @SuppressLint("RestrictedApi")
        @SuppressWarnings("ConstantConditions")
        void updatePreferencesLayout() {
            SharedPreferences p = getPreferenceManager().getSharedPreferences();
            if (!SamsungDexUtils.available())
                findPreference("dexMetaKeyCapture").setVisible(false);
            SeekBarPreference scalePreference = findPreference("displayScale");
            scalePreference.setMin(30);
            scalePreference.setMax(200);
            scalePreference.setSeekBarIncrement(10);
            scalePreference.setShowSeekBarValue(true);

            switch (p.getString("displayResolutionMode", "native")) {
                case "scaled":
                    findPreference("displayScale").setVisible(true);
                    findPreference("displayResolutionExact").setVisible(false);
                    findPreference("displayResolutionCustom").setVisible(false);
                    break;
                case "exact":
                    findPreference("displayScale").setVisible(false);
                    findPreference("displayResolutionExact").setVisible(true);
                    findPreference("displayResolutionCustom").setVisible(false);
                    break;
                case "custom":
                    findPreference("displayScale").setVisible(false);
                    findPreference("displayResolutionExact").setVisible(false);
                    findPreference("displayResolutionCustom").setVisible(true);
                    break;
                default:
                    findPreference("displayScale").setVisible(false);
                    findPreference("displayResolutionExact").setVisible(false);
                    findPreference("displayResolutionCustom").setVisible(false);
            }
            findPreference("hideEKOnVolDown").setEnabled(p.getBoolean("showAdditionalKbd", false));
            findPreference("dexMetaKeyCapture").setEnabled(!p.getBoolean("enableAccessibilityServiceAutomatically", false));
            findPreference("enableAccessibilityServiceAutomatically").setEnabled(!p.getBoolean("dexMetaKeyCapture", false));
            findPreference("filterOutWinkey").setEnabled(p.getBoolean("enableAccessibilityServiceAutomatically", false));

            if (Build.VERSION.SDK_INT < Build.VERSION_CODES.P)
                findPreference("hideCutout").setVisible(false);

            findPreference("displayResolutionMode").setSummary(p.getString("displayResolutionMode", "native"));
            findPreference("displayResolutionExact").setSummary(p.getString("displayResolutionExact", "1280x1024"));
            findPreference("displayResolutionCustom").setSummary(p.getString("displayResolutionCustom", "1280x1024"));
            findPreference("displayStretch").setEnabled("exact".contentEquals(p.getString("displayResolutionMode", "native")) || "custom".contentEquals(p.getString("displayResolutionMode", "native")));
            int modeValue = Integer.parseInt(p.getString("touchMode", "1")) - 1;
            String mode = getResources().getStringArray(R.array.touchscreenInputModesEntries)[modeValue];
            findPreference("touchMode").setSummary(mode);
            findPreference("scaleTouchpad").setVisible("1".equals(p.getString("touchMode", "1")) && !"native".equals(p.getString("displayResolutionMode", "native")));
            findPreference("showMouseHelper").setEnabled("1".equals(p.getString("touchMode", "1")));

            boolean requestNotificationPermissionVisible =
                Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU
                    && ContextCompat.checkSelfPermission(requireContext(), POST_NOTIFICATIONS) == PERMISSION_DENIED;
            findPreference("requestNotificationPermission").setVisible(requestNotificationPermissionVisible);
        }

        @Override
        public void onCreate(final Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            SharedPreferences preferences = getPreferenceManager().getSharedPreferences();

            String showImeEnabled = Settings.Secure.getString(requireActivity().getContentResolver(), SHOW_IME_WITH_HARD_KEYBOARD);
            if (showImeEnabled == null) showImeEnabled = "0";
            SharedPreferences.Editor p = Objects.requireNonNull(preferences).edit();
            p.putBoolean("showIMEWhileExternalConnected", showImeEnabled.contentEquals("1"));
            p.apply();

            setListeners(getPreferenceScreen());
            updatePreferencesLayout();
        }

        void setListeners(PreferenceGroup g) {
            for (int i = 0; i < g.getPreferenceCount(); i++) {
                g.getPreference(i).setOnPreferenceChangeListener(this);
                g.getPreference(i).setOnPreferenceClickListener(this);
                g.getPreference(i).setSingleLineTitle(false);

                if (g.getPreference(i) instanceof PreferenceGroup)
                    setListeners((PreferenceGroup) g.getPreference(i));
            }
        }

        @Override
        public boolean onPreferenceClick(@NonNull Preference preference) {
            if ("enableAccessibilityService".contentEquals(preference.getKey())) {
                Intent intent = new Intent(android.provider.Settings.ACTION_ACCESSIBILITY_SETTINGS);
                startActivityForResult(intent, 0);
            }

            if ("extra_keys_config".contentEquals(preference.getKey())) {
                @SuppressLint("InflateParams")
                View view = getLayoutInflater().inflate(R.layout.extra_keys_config, null, false);
                SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(getContext());
                EditText config = view.findViewById(R.id.extra_keys_config);
                config.setTypeface(Typeface.MONOSPACE);
                TextView desc = view.findViewById(R.id.extra_keys_config_description);
                desc.setLinksClickable(true);
                desc.setText(R.string.extra_keys_config_desc);
                desc.setMovementMethod(LinkMovementMethod.getInstance());
                new android.app.AlertDialog.Builder(getActivity())
                    .setView(view)
                    .setTitle("Extra keys config")
                    .setPositiveButton("OK",
                        (dialog, whichButton) -> {

                        }
                    )
                    .setNegativeButton("Cancel", (dialog, whichButton) -> dialog.dismiss())
                    .create()
                    .show();
            }
            if ("install_x11_server_bridge".contentEquals(preference.getKey())) {
                View view = getLayoutInflater().inflate(R.layout.x11_server_bridge_config, null, false);
                @SuppressLint({"MissingInflatedId", "LocalSuppress"})
                TextView desc = view.findViewById(R.id.x11_server_bridge_config_description);
                desc.setText(R.string.x11_server_bridge_config);
                desc.setMovementMethod(LinkMovementMethod.getInstance());
                new android.app.AlertDialog.Builder(getActivity())
                    .setView(view)
                    .setTitle("X11 server bridge installer")
                    .setPositiveButton("OK",
                        (dialog, whichButton) -> {
                            if (activity != null) {
                                activity.installX11ServerBridge();
                            }
                        }
                    )
                    .setNegativeButton("Cancel", (dialog, whichButton) -> dialog.dismiss())
                    .create()
                    .show();
            }
            if ("stop_desktop".contentEquals(preference.getKey())) {
                new android.app.AlertDialog.Builder(getActivity())
                    .setTitle("Stop Desktop")
                    .setPositiveButton("OK",
                        (dialog, whichButton) -> {
                            if (activity != null) {
                                activity.stopDesktop();
                            }
                        }
                    )
                    .setNegativeButton("Cancel", (dialog, whichButton) -> dialog.dismiss())
                    .create()
                    .show();
            }
            if ("open_keyboard".contentEquals(preference.getKey())) {
                activity.openSoftKeyboar();
            }
            if ("open_inputcontroller".contentEquals(preference.getKey())) {
                if (!activity.touchShow) {
                    activity.showInputControlsDialog();
                    activity.touchShow = true;
                } else {
                    activity.hideInputControls();
                    activity.touchShow = false;
                }
            }
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU && "requestNotificationPermission".contentEquals(preference.getKey()))
                ActivityCompat.requestPermissions(requireActivity(), new String[]{POST_NOTIFICATIONS}, 101);

            updatePreferencesLayout();
            return false;
        }

        @SuppressLint("ApplySharedPref")
        @Override
        public boolean onPreferenceChange(Preference preference, Object newValue) {
            String key = preference.getKey();
//            Log.d("Preferences", "changed preference: " + key+":"+newValue);
            handler.postDelayed(this::updatePreferencesLayout, 100);

            if ("showIMEWhileExternalConnected".contentEquals(key)) {
                boolean enabled = newValue.toString().contentEquals("true");
                try {
                    Settings.Secure.putString(requireActivity().getContentResolver(), SHOW_IME_WITH_HARD_KEYBOARD, enabled ? "1" : "0");
                } catch (Exception e) {
                    if (e instanceof SecurityException) {
                        new AlertDialog.Builder(requireActivity())
                            .setTitle("Permission denied")
                            .setMessage("Android requires WRITE_SECURE_SETTINGS permission to change this setting.\n" +
                                "Please, launch this command using ADB:\n" +
                                "adb shell pm grant com.termux.display android.permission.WRITE_SECURE_SETTINGS")
                            .setNegativeButton("OK", null)
                            .create()
                            .show();
                    } else e.printStackTrace();
                    return false;
                }
            }

            if ("displayScale".contentEquals(key)) {
                int scale = (Integer) newValue;
                if (scale % 10 != 0) {
                    scale = Math.round(((float) scale) / 10) * 10;
                    ((SeekBarPreference) preference).setValue(scale);
                    return false;
                }
            }

            if ("displayDensity".contentEquals(key)) {
                int v;
                try {
                    v = Integer.parseInt((String) newValue);
                } catch (NumberFormatException | PatternSyntaxException ignored) {
                    Toast.makeText(getActivity(), "This field accepts only numerics between 96 and 800", Toast.LENGTH_SHORT).show();
                    return false;
                }

                return (v > 96 && v < 800);
            }

            if ("displayResolutionCustom".contentEquals(key)) {
                String value = (String) newValue;
                try {
                    String[] resolution = value.split("x");
                    Integer.parseInt(resolution[0]);
                    Integer.parseInt(resolution[1]);
                } catch (NumberFormatException | PatternSyntaxException ignored) {
                    Toast.makeText(getActivity(), "Wrong resolution format", Toast.LENGTH_SHORT).show();
                    return false;
                }
            }
            if ("touch_sensitivity".contentEquals(key)) {
                int v;
                try {
                    v = (int) newValue;
                } catch (NumberFormatException | PatternSyntaxException ignored) {
                    Toast.makeText(getActivity(), "Wrong touch sensitivity", Toast.LENGTH_SHORT).show();
                    return false;
                }
                if (v<1||v>20){
                    return false;
                }
            }
            if ("showAdditionalKbd".contentEquals(key) && (Boolean) newValue) {
                SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(requireContext());
                SharedPreferences.Editor edit = preferences.edit();
                edit.putBoolean("additionalKbdVisible", true);
                edit.commit();
            }

            if ("enableAccessibilityServiceAutomatically".contentEquals(key)) {
                if (!((Boolean) newValue))
                    KeyInterceptor.shutdown();
                if (requireContext().checkSelfPermission(WRITE_SECURE_SETTINGS) != PERMISSION_GRANTED) {
                    new AlertDialog.Builder(requireContext())
                        .setTitle("Permission denied")
                        .setMessage("Android requires WRITE_SECURE_SETTINGS permission to start accessibility service automatically.\n" +
                            "Please, launch this command using ADB:\n" +
                            "adb shell pm grant com.termux.display android.permission.WRITE_SECURE_SETTINGS")
                        .setNegativeButton("OK", null)
                        .create()
                        .show();
                    return false;
                }
            }

            Intent intent = new Intent(ACTION_PREFERENCES_CHANGED);
            intent.putExtra("key", key);
            intent.setPackage("com.termux");
            requireContext().sendBroadcast(intent);

            handler.postAtTime(this::updatePreferencesLayout, 100);
            return true;
        }
    }

    private void openSoftKeyboar() {
        termuxActivityListener.openSoftwareKeyboard();
    }

    public static class Receiver extends BroadcastReceiver {
        public Receiver() {
            super();
        }

        @Override
        public IBinder peekService(Context myContext, Intent service) {
            return super.peekService(myContext, service);
        }

        @SuppressLint("ApplySharedPref")
        @Override
        public void onReceive(Context context, Intent intent) {
            try {
                if (intent.getExtras() != null) {
                    SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(context);
                    if (intent.getStringExtra("list") != null) {
                        String result = "";
                        try {
                            XmlResourceParser parser = context.getResources().getXml(R.xml.preferences);
                            String namespace = "http://schemas.android.com/apk/res/android";
                            Map<String, ?> p = preferences.getAll();
                            int eventType = parser.getEventType();
                            while (eventType != XmlPullParser.END_DOCUMENT) {
                                if (eventType == XmlPullParser.START_TAG) {
                                    String tagName = parser.getName();

                                    if (tagName.contains("Preference") && !tagName.equals("PreferenceScreen") && !tagName.equals("PreferenceCategory")) {
                                        String key = parser.getAttributeValue(namespace, "key");
                                        String value = p.containsKey(key) ? Objects.requireNonNull(p.get(key)).toString() : parser.getAttributeValue(namespace, "defaultValue");
                                        if (key.equals("extra_keys_config") && !p.containsKey(key))
                                            if (key.equals("touchMode")) {
                                                String[] options0 = context.getResources().getStringArray(R.array.touchscreenInputModesEntries);
                                                String[] options1 = context.getResources().getStringArray(R.array.touchscreenInputModesValues);
                                                for (int i = 0; i < options0.length; i++)
                                                    if (value.contentEquals(options1[i]))
                                                        value = options0[i];
                                            }

                                        //noinspection StringConcatenationInLoop
                                        result += "\"" + key + "\"=\"" + value + "\"\n";
                                    }
                                }
                                eventType = parser.next();
                            }
                        } catch (XmlPullParserException | IOException e) {
                            throw new RuntimeException(e);
                        }
                        setResultCode(2);
                        setResultData(result);

                        return;
                    }

                    SharedPreferences.Editor edit = preferences.edit();
                    for (String key : intent.getExtras().keySet()) {
                        String newValue = intent.getStringExtra(key);
                        if (newValue == null)
                            continue;

                        switch (key) {
                            case "showIMEWhileExternalConnected": {
                                boolean enabled = "true".contentEquals(newValue);
                                try {
                                    Settings.Secure.putString(context.getContentResolver(), SHOW_IME_WITH_HARD_KEYBOARD, enabled ? "1" : "0");
                                } catch (Exception e) {
                                    if (e instanceof SecurityException) {
                                        setResultCode(1);
                                        setResultData("Permission denied.\n" +
                                            "Android requires WRITE_SECURE_SETTINGS permission to change `show_ime_with_hard_keyboard` setting.\n" +
                                            "Please, launch this command using ADB:\n" +
                                            "adb shell pm grant com.termux.display android.permission.WRITE_SECURE_SETTINGS");
                                        return;
                                    } else e.printStackTrace();
                                }
                                break;
                            }
                            case "displayScale": {
                                int scale = Integer.parseInt(newValue);
                                if (scale % 10 != 0) {
                                    scale = Math.round(((float) scale) / 10) * 10;
                                    edit.putInt("displayScale", scale);
                                } else
                                    edit.putInt("displayScale", scale);
                                break;
                            }
                            case "displayDensity": {
                                int v;
                                try {
                                    v = Integer.parseInt(newValue);
                                } catch (NumberFormatException | PatternSyntaxException ignored) {
                                    v = 0;
                                }

                                if (!(v > 96 && v < 800)) {
                                    setResultCode(1);
                                    setResultData("displayDensity accepts only numerics between 96 and 800.");
                                    return;
                                }

                                edit.putInt("displayDensity", v);
                                break;
                            }
                            case "displayResolutionCustom": {
                                try {
                                    String[] resolution = newValue.split("x");
                                    Integer.parseInt(resolution[0]);
                                    Integer.parseInt(resolution[1]);
                                } catch (NumberFormatException | PatternSyntaxException ignored) {
                                    setResultCode(1);
                                    setResultData("displayResolutionCustom: Wrong resolution format.");
                                    return;
                                }

                                edit.putString("displayResolutionCustom", newValue);
                                break;
                            }
                            case "showAdditionalKbd": {
                                if ("true".contentEquals(newValue))
                                    edit.putBoolean("additionalKbdVisible", true);
                                edit.putBoolean("showAdditionalKbd", "true".contentEquals(newValue));
                                break;
                            }
                            case "enableAccessibilityServiceAutomatically": {
                                if (!"true".equals(newValue))
                                    KeyInterceptor.shutdown();
                                else if (context.checkSelfPermission(WRITE_SECURE_SETTINGS) != PERMISSION_GRANTED) {
                                    setResultCode(1);
                                    setResultData("Permission denied.\n" +
                                        "Android requires WRITE_SECURE_SETTINGS permission to change `show_ime_with_hard_keyboard` setting.\n" +
                                        "Please, launch this command using ADB:\n" +
                                        "adb shell pm grant com.termux.display android.permission.WRITE_SECURE_SETTINGS");
                                    return;
                                }

                                edit.putBoolean("enableAccessibilityServiceAutomatically", "true".contentEquals(newValue));
                                break;
                            }
                            case "extra_keys_config": {
                                edit.putString(key, newValue);
                                break;
                            }

                            case "displayResolutionMode":
                            case "displayResolutionExact": {
                                int array = 0;
                                switch (key) {
                                    case "displayResolutionMode":
                                        array = R.array.displayResolutionVariants;
                                        break;
                                    case "displayResolutionExact":
                                        array = R.array.displayResolution;
                                        break;
                                }
                                String[] options = context.getResources().getStringArray(array);
                                if (!Arrays.asList(options).contains(newValue)) {
                                    setResultCode(1);
                                    setResultData(key + ": can not be set to " + newValue);
                                    return;
                                }
                                edit.putString(key, newValue);
                                break;
                            }
                            case "touchMode": {
                                String[] options0 = context.getResources().getStringArray(R.array.touchscreenInputModesEntries);
                                String[] options1 = context.getResources().getStringArray(R.array.touchscreenInputModesValues);
                                boolean found = false;
                                for (int i = 0; i < options0.length; i++) {
                                    if (newValue.contentEquals(options0[i])) {
                                        found = true;
                                        edit.putString(key, options1[i]);
                                        break;
                                    }
                                }

                                if (!found) {
                                    setResultCode(1);
                                    setResultData(key + ": can not be set to " + newValue);
                                    return;
                                }
                                break;
                            }
                            case "touch_sensitivity":
                            case "displayStretch":
                            case "Reseed":
                            case "PIP":
                            case "fullscreen":
                            case "forceLandscape":
                            case "hideCutout":
                            case "keepScreenOn":
                            case "scaleTouchpad":
                            case "showStylusClickOverride":
                            case "showMouseHelper":
                            case "pointerCapture":
                            case "tapToMove":
                            case "preferScancodes":
                            case "dexMetaKeyCapture":
                            case "filterOutWinkey":
                            case "clipboardSync": {
                                edit.putBoolean(key, "true".contentEquals(newValue));
                                break;
                            }
                            default: {
                                setResultCode(4);
                                setResultData(key + ": unrecognised option");
                                return;
                            }
                        }

                        Intent intent0 = new Intent(ACTION_PREFERENCES_CHANGED);
                        intent0.putExtra("key", key);
                        intent0.putExtra("fromBroadcast", true);
                        intent0.setPackage("com.termux");
                        context.sendBroadcast(intent0);
                    }
                    edit.commit();
                }

                setResultCode(2);
                setResultData("Done");
            } catch (Exception e) {
                setResultCode(4);
                StringWriter sw = new StringWriter();
                PrintWriter pw = new PrintWriter(sw);
                e.printStackTrace(pw);
                setResultData(sw.toString());
            }
        }
    }

    public static Handler handler = new Handler();

    //inout control
    public InputControlsView getInputControlsView() {
        return inputControlsView;
    }

    public WinHandler getWinHandler() {
        return null;
    }

    public InputControlsManager getInputControlsManager() {
        return inputControlsManager;
    }

    public void setOpenFileCallback(Callback<Uri> openFileCallback) {
        this.openFileCallback = openFileCallback;
    }

    public String getControlsProfile() {
        return controlsProfile;
    }

    public Shortcut getShortcut() {
        return shortcut;
    }

    public DownloadProgressDialog getPreloaderDialog() {
        return preloaderDialog;
    }

    public void showInputControlsDialog() {
        final ContentDialog dialog = new ContentDialog(this, R.layout.input_controls_dialog);
        dialog.setTitle(R.string.input_controls);
        dialog.setIcon(R.drawable.icon_input_controls);

        final Spinner sProfile = dialog.findViewById(R.id.SProfile);
        Runnable loadProfileSpinner = () -> {
            ArrayList<ControlsProfile> profiles = inputControlsManager.getProfiles();
            ArrayList<String> profileItems = new ArrayList<>();
            int selectedPosition = 0;
            profileItems.add("-- " + getString(R.string.disabled) + " --");
            for (int i = 0; i < profiles.size(); i++) {
                ControlsProfile profile = profiles.get(i);
                if (profile == inputControlsView.getProfile()) selectedPosition = i + 1;
                profileItems.add(profile.getName());
            }

            sProfile.setAdapter(new ArrayAdapter<>(this, android.R.layout.simple_spinner_dropdown_item, profileItems));
            sProfile.setSelection(selectedPosition);
        };
        loadProfileSpinner.run();

        final CheckBox cbLockCursor = dialog.findViewById(R.id.CBLockCursor);
        cbLockCursor.setChecked(xServer.cursorLocker.getState() == CursorLocker.State.LOCKED);

        final CheckBox cbShowTouchscreenControls = dialog.findViewById(R.id.CBShowTouchscreenControls);
        cbShowTouchscreenControls.setChecked(inputControlsView.isShowTouchscreenControls());

        dialog.findViewById(R.id.BTSettings).setOnClickListener((v) -> {
            int position = sProfile.getSelectedItemPosition();
            Intent intent = new Intent(this, InputControllerActivity.class);
            intent.putExtra("edit_input_controls", true);
            intent.putExtra("selected_profile_id", position > 0 ? inputControlsManager.getProfiles().get(position - 1).id : 0);
            editInputControlsCallback = () -> {
                hideInputControls();
                inputControlsManager.loadProfiles();
                loadProfileSpinner.run();
            };
            intent.putExtra("set_orientation", orientation);
            startActivityForResult(intent, InputControllerActivity.EDIT_INPUT_CONTROLS_REQUEST_CODE);
        });

        dialog.setOnConfirmCallback(() -> {
            if (termuxActivityListener == null) {
                return;
            }
            xServer.cursorLocker.setState(cbLockCursor.isChecked() ? CursorLocker.State.LOCKED : CursorLocker.State.CONFINED);
            inputControlsView.setShowTouchscreenControls(cbShowTouchscreenControls.isChecked());
            int position = sProfile.getSelectedItemPosition();
            if (position > 0) {
                showInputControls(inputControlsManager.getProfiles().get(position - 1));
            } else hideInputControls();
        });

        dialog.show();
    }

    protected void showInputControls(ControlsProfile controlsProfile) {
        inputControlsView.setVisibility(View.VISIBLE);
        inputControlsView.requestFocus();
        inputControlsView.setProfile(controlsProfile);
//        touchpadView.setSensitivity(controlsProfile.getCursorSpeed() * globalCursorSpeed);
//        touchpadView.setPointerButtonRightEnabled(false);

        inputControlsView.invalidate();
    }

    protected void hideInputControls() {
        inputControlsView.setShowTouchscreenControls(true);
        inputControlsView.setVisibility(View.GONE);
        inputControlsView.setProfile(null);

//        touchpadView.setSensitivity(globalCursorSpeed);
//        touchpadView.setPointerButtonLeftEnabled(true);
//        touchpadView.setPointerButtonRightEnabled(true);

        inputControlsView.invalidate();
    }
}
