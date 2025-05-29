package com.termux.x11;

import static android.Manifest.permission.POST_NOTIFICATIONS;
import static android.Manifest.permission.WRITE_SECURE_SETTINGS;
import static android.content.pm.PackageManager.PERMISSION_DENIED;
import static android.content.pm.PackageManager.PERMISSION_GRANTED;
import static android.os.Build.VERSION.SDK_INT;
import static android.system.Os.getuid;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.content.res.XmlResourceParser;
import android.graphics.Typeface;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;
import android.preference.PreferenceManager;
import android.provider.Settings;
import android.text.method.LinkMovementMethod;
import android.util.Log;
import android.util.TypedValue;
import android.view.Display;
import android.view.MenuItem;
import android.view.View;
import android.view.WindowManager;
import android.widget.ArrayAdapter;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.core.util.Consumer;
import androidx.preference.Preference;
import androidx.preference.Preference.OnPreferenceChangeListener;
import androidx.preference.PreferenceDataStore;
import androidx.preference.PreferenceFragmentCompat;
import androidx.preference.PreferenceGroup;
import androidx.preference.SeekBarPreference;

import com.termux.x11.controller.InputControllerActivity;
import com.termux.x11.controller.container.Container;
import com.termux.x11.controller.container.Shortcut;
import com.termux.x11.controller.contentdialog.ContentDialog;
import com.termux.x11.controller.core.Callback;
import com.termux.x11.controller.core.DownloadProgressDialog;
import com.termux.x11.controller.inputcontrols.ControlsProfile;
import com.termux.x11.controller.inputcontrols.InputControlsManager;
import com.termux.x11.controller.widget.InputControlsView;
import com.termux.x11.controller.widget.TouchpadView;
import com.termux.x11.controller.winhandler.ProcessInfo;
import com.termux.x11.controller.winhandler.WinHandler;
import com.termux.x11.utils.KeyInterceptor;
import com.termux.x11.utils.SamsungDexUtils;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Scanner;
import java.util.Set;
import java.util.regex.PatternSyntaxException;

@SuppressWarnings("deprecation")
public class LoriePreferences extends AppCompatActivity {
    public static int OPEN_FILE_REQUEST_CODE = 102;
    static final String ACTION_PREFERENCES_CHANGED = "com.termux.x11.ACTION_PREFERENCES_CHANGED";
    public static Prefs prefs = null;
    static final String SHOW_IME_WITH_HARD_KEYBOARD = "show_ime_with_hard_keyboard";
    protected LoriePreferenceFragment loriePreferenceFragment;
    protected LorieView xServer;
    protected static boolean touchShow = false;
    protected int orientation = ActivityInfo.SCREEN_ORIENTATION_PORTRAIT;

    public boolean getTouchShow() {
        return touchShow;
    }

    public List<ProcessInfo> getTermuxProcessorInfo(String tag) {
        if (termuxActivityListener != null) {
            return termuxActivityListener.collectProcessorInfo(tag);
        }
        return null;
    }

    protected interface TermuxActivityListener {
        void onX11PreferenceSwitchChange(boolean isOpen);

        void releaseSlider(boolean open);

        void onChangeOrientation(int landscape);

        void reInstallX11StartScript(Activity activity);

        void stopDesktop(Activity activity);

        void openSoftwareKeyboard();

        void showProcessManager();

        void changePreference(String key);

        List<ProcessInfo> collectProcessorInfo(String tag);

        void setFloatBallMenu(boolean enableFloatBallMenu, boolean enableGlobalFloatBallMenu);
    }

    public TermuxActivityListener getTermuxActivityListener() {
        return termuxActivityListener;
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
    protected TouchpadView touchpadView;
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
        prefs = new Prefs(this);
        loriePreferenceFragment = new LoriePreferenceFragment();
//        getSupportFragmentManager().beginTransaction().replace(android.R.id.content, loriePreferenceFragment).commit();

        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            actionBar.setDisplayHomeAsUpEnabled(true);
            actionBar.setHomeButtonEnabled(true);
            actionBar.setTitle("Preferences");
        }
        loriePreferenceFragment.setPreferenceActivity(this);
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
        private LoriePreferences preferenceActivity;

        public void setPreferenceActivity(LoriePreferences preferenceActivity) {
            this.preferenceActivity = preferenceActivity;
        }

        private final Runnable updateLayout = this::updatePreferencesLayout;
        private static final Method onSetInitialValue;
        static {
            try {
                //noinspection JavaReflectionMemberAccess
                onSetInitialValue = Preference.class.getMethod("onSetInitialValue", boolean.class, Object.class);
            } catch (NoSuchMethodException e) {
                throw new RuntimeException(e);
            }
        }

        void onSetInitialValue(Preference p) {
            try {
                onSetInitialValue.invoke(p, false, null);
            } catch (IllegalAccessException | InvocationTargetException e) {
                throw new RuntimeException(e);
            }
        }

        final String root;
        /** @noinspection unused*/ // Used by `androidx.fragment.app.Fragment.instantiate`...
        public LoriePreferenceFragment() {
            this(null);
        }

        public LoriePreferenceFragment(String root) {
            this.root = root;
        }

        @Override
        public void onResume() {
            super.onResume();
            //noinspection DataFlowIssue
            ((LoriePreferences) getActivity()).getSupportActionBar().setTitle(getPreferenceScreen().getTitle());
        }

        /** @noinspection SameParameterValue*/
        private void with(CharSequence key, Consumer<Preference> action) {
            Preference p = findPreference(key);
            if (p != null)
                action.accept(p);
        }

        @SuppressLint("DiscouragedApi")
        int findId(String name) {
            //noinspection DataFlowIssue
            return getResources().getIdentifier("pref_" + name, "string", getContext().getPackageName());
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
            if (preferenceActivity.touchShow) {
                findPreference("select_controller").setTitle(R.string.close_controller);
            } else {
                findPreference("select_controller").setTitle(R.string.open_controller);
            }
            boolean enableFloatBallMenu = p.getBoolean("enableFloatBallMenu", false);
            if (!enableFloatBallMenu) {
                findPreference("enableGlobalFloatBallMenu").setEnabled(false);
                findPreference("enableGlobalFloatBallMenu").setVisible(false);
                findPreference("stop_desktop").setEnabled(true);
                findPreference("stop_desktop").setVisible(true);
                findPreference("open_keyboard").setEnabled(true);
                findPreference("open_keyboard").setVisible(true);
                findPreference("select_controller").setEnabled(true);
                findPreference("select_controller").setVisible(true);
                findPreference("open_progress_manager").setVisible(true);
                findPreference("open_progress_manager").setVisible(true);
            } else {
                findPreference("enableGlobalFloatBallMenu").setVisible(true);
                findPreference("enableGlobalFloatBallMenu").setEnabled(true);
                findPreference("stop_desktop").setEnabled(false);
                findPreference("stop_desktop").setVisible(false);
                findPreference("open_keyboard").setEnabled(false);
                findPreference("open_keyboard").setVisible(false);
                findPreference("select_controller").setEnabled(false);
                findPreference("select_controller").setVisible(false);
                findPreference("open_progress_manager").setVisible(false);
                findPreference("open_progress_manager").setVisible(false);
            }

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
                            if (preferenceActivity != null) {
                                preferenceActivity.installX11ServerBridge();
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
                            if (preferenceActivity != null) {
                                preferenceActivity.stopDesktop();
                            }
                        }
                    )
                    .setNegativeButton("Cancel", (dialog, whichButton) -> dialog.dismiss())
                    .create()
                    .show();
            }
            if ("open_keyboard".contentEquals(preference.getKey())) {
                preferenceActivity.openSoftKeyBoar();
            }
            if ("select_controller".contentEquals(preference.getKey())) {
                if (!preferenceActivity.touchShow) {
                    preferenceActivity.showInputControlsDialog();
                } else {
                    preferenceActivity.hideInputControls();
                }
            }
            if ("open_progress_manager".contentEquals(preference.getKey())) {
                preferenceActivity.callProgressManager();
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
                                "adb shell pm grant com.termux.x11 android.permission.WRITE_SECURE_SETTINGS")
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
                if (v < 1 || v > 20) {
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
                    KeyInterceptor.shutdown(false);
                if (requireContext().checkSelfPermission(WRITE_SECURE_SETTINGS) != PERMISSION_GRANTED) {
                    new AlertDialog.Builder(requireContext())
                        .setTitle("Permission denied")
                        .setMessage("Android requires WRITE_SECURE_SETTINGS permission to start accessibility service automatically.\n" +
                            "Please, launch this command using ADB:\n" +
                            "adb shell pm grant com.termux.x11 android.permission.WRITE_SECURE_SETTINGS")
                        .setNegativeButton("OK", null)
                        .create()
                        .show();
                    return false;
                }
            }

//            Intent intent = new Intent(ACTION_PREFERENCES_CHANGED);
//            intent.putExtra("key", key);
//            intent.setPackage("com.termux");
//            requireContext().sendBroadcast(intent);
            handler.postAtTime(new Runnable() {
                @Override
                public void run() {
                    if (preferenceActivity.termuxActivityListener != null) {
                        updatePreferencesLayout();
                        preferenceActivity.termuxActivityListener.changePreference(key);
                    }
                }
            }, 1000);
//            handler.postAtTime(this::updatePreferencesLayout, 100);
            return true;
        }
    }

    private void callProgressManager() {
        if (termuxActivityListener != null) {
            termuxActivityListener.showProcessManager();
        }
    }

    private void openSoftKeyBoar() {
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

        /** @noinspection StringConcatenationInLoop*/
        @SuppressLint("ApplySharedPref")
        @Override
        public void onReceive(Context context, Intent intent) {
            Bundle bundle = intent != null ? intent.getBundleExtra(null) : null;
            IBinder ibinder = bundle != null ? bundle.getBinder(null) : null;
            IRemoteCmdImterface remote = ibinder != null ? IRemoteCmdImterface.Stub.asInterface(ibinder) : null;

            try {
                if (intent != null && intent.getExtras() != null) {
                    Prefs p = (MainActivity.getInstance() != null) ? new Prefs(MainActivity.getInstance()) : (prefs != null ? prefs : new Prefs(context));
                    if (intent.getStringExtra("list") != null) {
                        String result = "";
                        for (PrefsProto.Preference pref : p.keys.values()) {
                            if (pref.type == String.class)
                                result += "\"" + pref.key + "\"=\"" + pref.asString().get() + "\"\n";
                            else if (pref.type == int.class)
                                result += "\"" + pref.key + "\"=\"" + pref.asInt().get() + "\"\n";
                            else if (pref.type == boolean.class)
                                result += "\"" + pref.key + "\"=\"" + pref.asBoolean().get() + "\"\n";
                            else if (pref.type == String[].class) {
                                String[] entries = context.getResources().getStringArray(pref.asList().entries);
                                String[] values = context.getResources().getStringArray(pref.asList().values);
                                String value = pref.asList().get();
                                int index = Arrays.asList(values).indexOf(value);
                                if (index != -1)
                                    value = entries[index];
                                result += "\"" + pref.key + "\"=\"" + value + "\"\n";
                            }
                        }

                        sendResponse(remote, 0, 2, result.substring(0, result.length() - 1));
                        return;
                    }

                    SharedPreferences.Editor edit = p.get().edit();
                    for (String key : intent.getExtras().keySet()) {
                        if (key == null)
                            continue;
                        String newValue = intent.getStringExtra(key);
                        if (newValue == null)
                            continue;

                        switch (key) {
                            case "displayResolutionCustom": {
                                try {
                                    String[] resolution = newValue.split("x");
                                    Integer.parseInt(resolution[0]);
                                    Integer.parseInt(resolution[1]);
                                } catch (NumberFormatException | PatternSyntaxException ignored) {
                                    sendResponse(remote, 1, 1, "displayResolutionCustom: Wrong resolution format.");
                                    return;
                                }

                                edit.putString("displayResolutionCustom", newValue);
                                break;
                            }
                            case "enableAccessibilityServiceAutomatically": {
                                if (!"true".equals(newValue))
                                    KeyInterceptor.shutdown(false);
                                else if (context.checkSelfPermission(WRITE_SECURE_SETTINGS) != PERMISSION_GRANTED) {
                                    sendResponse(remote, 1, 1, "Permission denied.\n" +
                                        "Android requires WRITE_SECURE_SETTINGS permission to change `enableAccessibilityServiceAutomatically` setting.\n" +
                                        "Please, launch this command using ADB:\n" +
                                        "adb shell pm grant com.termux.x11 android.permission.WRITE_SECURE_SETTINGS");
                                    return;
                                }

                                edit.putBoolean("enableAccessibilityServiceAutomatically", "true".contentEquals(newValue));
                                break;
                            }
                            case "extra_keys_config": {
                                edit.putString(key, newValue);
                                break;
                            }
                            default: {
                                PrefsProto.Preference pref = p.keys.get(key);
                                if (pref != null && pref.type == boolean.class) {
                                    edit.putBoolean(key, "true".contentEquals(newValue));
                                    if ("showAdditionalKbd".contentEquals(key) && "true".contentEquals(newValue))
                                        edit.putBoolean("additionalKbdVisible", true);
                                } else if (pref != null && pref.type == int.class) {
                                    try {
                                        edit.putInt(key, Integer.parseInt(newValue));
                                    } catch (NumberFormatException | PatternSyntaxException exception) {
                                        sendResponse(remote, 1, 4, key + ": failed to parse integer: " + exception);
                                        return;
                                    }
                                } else if (pref != null && pref.type == String[].class) {
                                    PrefsProto.ListPreference _p = (PrefsProto.ListPreference) pref;
                                    String[] entries = _p.getEntries();
                                    String[] values = _p.getValues();
                                    int index = Arrays.asList(entries).indexOf(newValue);

                                    if (index == -1 && _p.entries != _p.values)
                                        index = Arrays.asList(values).indexOf(newValue);

                                    if (index != -1) {
                                        edit.putString(key, values[index]);
                                        break;
                                    }

                                    sendResponse(remote, 1, 1, key + ": can not be set to \"" + newValue + "\", possible options are " + Arrays.toString(entries) + (_p.entries != _p.values ? " or " + Arrays.toString(values) : ""));
                                    return;
                                } else {
                                    sendResponse(remote, 1, 4, key + ": unrecognised option");
                                    return;
                                }
                            }
                        }

                        Intent intent0 = new Intent(ACTION_PREFERENCES_CHANGED);
                        intent0.putExtra("key", key);
                        intent0.putExtra("fromBroadcast", true);
                        intent0.setPackage("com.termux.x11");
                        context.sendBroadcast(intent0);
                    }
                    edit.commit();
                }

                sendResponse(remote, 0, 2, "Done");
            } catch (Exception e) {
                sendResponse(remote, 1, 4, e.toString());
            }
        }

        void sendResponse(IRemoteCmdImterface remote, int status, int oldStatus, String text) {
            if (remote != null) {
                try {
                    remote.exit(status, text);
                } catch (RemoteException ex) {
                    Log.e("LoriePreferences", "Failed to send response to commandline proxy", ex);
                }
            } else if (isOrderedBroadcast()) {
                setResultCode(oldStatus);
                setResultData(text);
            }
        }

        // For changing preferences from commandline
        private static final IBinder iface = new IRemoteCmdImterface.Stub() {
            @Override
            public void exit(int code, String output) {
                System.out.println(output);
                CmdEntryPoint.handler.post(() -> System.exit(code));
            }
        };

        private static void help() {
            System.err.print("termux-x11-preference [list] {key:value} [{key2:value2}]...");
            System.exit(0);
        }

        @Keep
        @SuppressLint("WrongConstant")
        public static void main(String[] args) {
            android.util.Log.i("LoriePreferences$Receiver", "commit " + BuildConfig.COMMIT);
            //noinspection resource
            ParcelFileDescriptor in = ParcelFileDescriptor.adoptFd(0);
            Intent i = new Intent("com.termux.x11.CHANGE_PREFERENCE");
            Bundle bundle = new Bundle();
            boolean inputIsFile = !android.system.Os.isatty(in.getFileDescriptor());

            in.detachFd();
            bundle.putBinder(null, iface);
            i.setPackage("com.termux.x11");
            i.putExtra(null, bundle);
            if (getuid() == 0 || getuid() == 2000)
                i.setFlags(0x00400000 /* FLAG_RECEIVER_FROM_SHELL */);

            if (inputIsFile && System.in != null) {
                Scanner scanner = new Scanner(System.in);
                String line;
                String[] v;
                while (scanner.hasNextLine()) {
                    line = scanner.nextLine();
                    if (!line.contains("="))
                        help();

                    v = line.split("=");
                    if (v[0].startsWith("\"") && v[0].endsWith("\""))
                        v[0] = v[0].substring(1, v[0].length() - 1);
                    if (v[1].startsWith("\"") && v[1].endsWith("\""))
                        v[1] = v[1].substring(1, v[1].length() - 1);
                    i.putExtra(v[0], v[1]);
                }
            }

            for (String a: args) {
                if ("list".equals(a)) {
                    i.putExtra("list", "");
                } else if (a != null && a.contains(":")) {
                    String[] v = a.split(":");
                    i.putExtra(v[0], v[1]);
                } else
                    help();
            }

            CmdEntryPoint.handler.post(() -> CmdEntryPoint.sendBroadcast(i));
            CmdEntryPoint.handler.postDelayed(() -> {
                System.err.println("Failed to obtain response from app.");
                System.exit(1);
            }, 5000);
            Looper.loop();
        }
    }

    public static Handler handler = Looper.getMainLooper() != null ? new Handler(Looper.getMainLooper()) : null;

    public void onClick(View view) {
//        showFragment(new LoriePreferenceFragment("ekbar"));
        termuxActivityListener.onX11PreferenceSwitchChange(true);
    }

    /** @noinspection unused*/
    @SuppressLint("ApplySharedPref")
    public static class PrefsProto extends PreferenceDataStore {
        public static class Preference {
            protected final String key;
            protected final Class<?> type;
            protected final Object defValue;
            protected Preference(String key, Class<?> class_, Object default_) {
                this.key = key;
                this.type = class_;
                this.defValue = default_;
            }

            public ListPreference asList() {
                return (ListPreference) this;
            }

            public StringPreference asString() {
                return (StringPreference) this;
            }

            public IntPreference asInt() {
                return (IntPreference) this;
            }

            public BooleanPreference asBoolean() {
                return (BooleanPreference) this;
            }
        }

        public class BooleanPreference extends Preference {
            public BooleanPreference(String key, boolean defValue) {
                super(key, boolean.class, defValue);
            }

            public boolean get() {
                if ("storeSecondaryDisplayPreferencesSeparately".contentEquals(key))
                    return builtInDisplayPreferences.getBoolean(key, (boolean) defValue);

                return preferences.getBoolean(key, (boolean) defValue);
            }

            public void put(boolean v) {
                if ("storeSecondaryDisplayPreferencesSeparately".contentEquals(key)) {
                    builtInDisplayPreferences.edit().putBoolean(key, v).commit();
                    recheckStoringSecondaryDisplayPreferences();
                }

                preferences.edit().putBoolean(key, v).commit();
            }
        }

        public class IntPreference extends Preference {
            public IntPreference(String key, int defValue) {
                super(key, int.class, defValue);
            }

            public int get() {
                return preferences.getInt(key, (int) defValue);
            }

            public int defValue() {
                return preferences.getInt(key, (int) defValue);
            }
        }

        public class StringPreference extends Preference {
            public StringPreference(String key, String defValue) {
                super(key, String.class, defValue);
            }

            public String get() {
                return preferences.getString(key, (String) defValue);
            }

            public void put(String v) {
                preferences.edit().putString(key, v).commit();
            }
        }

        public class ListPreference extends Preference {
            private final int entries, values;

            public ListPreference(String key, String defValue, int entries, int values) {
                super(key, String[].class, defValue);
                this.entries = entries;
                this.values = values;
            }

            public String get() {
                return preferences.getString(key, (String) defValue);
            }

            public void put(String v) {
                preferences.edit().putString(key, v).commit();
            }

            public String[] getEntries() {
                return getArrayItems(entries, ctx.getResources());
            }

            public String[] getValues() {
                return getArrayItems(values, ctx.getResources());
            }

            private String[] getArrayItems(int resourceId, Resources resources) {
                ArrayList<String> itemList = new ArrayList<>();
                try(TypedArray typedArray = resources.obtainTypedArray(resourceId)) {
                    for (int i = 0; i < typedArray.length(); i++) {
                        int type = typedArray.getType(i);
                        if (type == TypedValue.TYPE_STRING) {
                            itemList.add(typedArray.getString(i));
                        } else if (type == TypedValue.TYPE_REFERENCE) {
                            int resIdOfArray = typedArray.getResourceId(i, 0);
                            itemList.addAll(Arrays.asList(resources.getStringArray(resIdOfArray)));
                        }
                    }
                }

                Object[] objectArray = itemList.toArray();
                return Arrays.copyOf(objectArray, objectArray.length, String[].class);
            }

        }

        static boolean storeSecondaryDisplayPreferencesSeparately = false;
        protected Context ctx;
        protected SharedPreferences preferences;
        protected SharedPreferences builtInDisplayPreferences;
        protected SharedPreferences secondaryDisplayPreferences;

        private PrefsProto() {} // No instantiation allowed
        protected PrefsProto(Context ctx) {
            this.ctx = ctx;
            builtInDisplayPreferences = PreferenceManager.getDefaultSharedPreferences(ctx);
            secondaryDisplayPreferences = ctx.getSharedPreferences("secondary", Context.MODE_PRIVATE);
            recheckStoringSecondaryDisplayPreferences();
        }

        protected void recheckStoringSecondaryDisplayPreferences() {
            storeSecondaryDisplayPreferencesSeparately = builtInDisplayPreferences.getBoolean("storeSecondaryDisplayPreferencesSeparately", false);
            boolean isExternalDisplay = ((WindowManager) ctx.getSystemService(WINDOW_SERVICE)).getDefaultDisplay().getDisplayId() != Display.DEFAULT_DISPLAY;
            preferences = (storeSecondaryDisplayPreferencesSeparately && isExternalDisplay) ? secondaryDisplayPreferences : builtInDisplayPreferences;
        }

        @Override public void putBoolean(String k, boolean v) {
            if ("storeSecondaryDisplayPreferencesSeparately".contentEquals(k)) {
                builtInDisplayPreferences.edit().putBoolean(k, v).commit();
                recheckStoringSecondaryDisplayPreferences();
            } else
                preferences.edit().putBoolean(k, v).commit();
        }
        @Override public boolean getBoolean(String k, boolean d) {
            if ("storeSecondaryDisplayPreferencesSeparately".contentEquals(k))
                return builtInDisplayPreferences.getBoolean(k, d);
            return preferences.getBoolean(k, d);
        }
        @Override public void putString(String k, @Nullable String v) { prefs.get().edit().putString(k, v).commit(); }
        @Override public void putStringSet(String k, @Nullable Set<String> v) { prefs.get().edit().putStringSet(k, v).commit(); }
        @Override public void putInt(String k, int v) { prefs.get().edit().putInt(k, v).commit(); }
        @Override public void putLong(String k, long v) { prefs.get().edit().putLong(k, v).commit(); }
        @Override public void putFloat(String k, float v) { prefs.get().edit().putFloat(k, v).commit(); }
        @Nullable @Override public String getString(String k, @Nullable String d) { return prefs.get().getString(k, d); }
        @Nullable @Override public Set<String> getStringSet(String k, @Nullable Set<String> ds) { return prefs.get().getStringSet(k, ds); }
        @Override public int getInt(String k, int d) { return prefs.get().getInt(k, d); }
        @Override public long getLong(String k, long d) { return prefs.get().getLong(k, d); }
        @Override public float getFloat(String k, float d) { return prefs.get().getFloat(k, d); }

        public SharedPreferences get() {
            return preferences;
        }

        public boolean isSecondaryDisplayPreferences() {
            return preferences == secondaryDisplayPreferences;
        }
    }

    //inout control
    public InputControlsView getInputControlsView() {
        return inputControlsView;
    }

    protected WinHandler winHandler;

    public WinHandler getWinHandler() {
        return winHandler;
    }

    public void setWinHandler(WinHandler handler) {
        this.winHandler = handler;
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
        cbLockCursor.setChecked(xServer.cursorLocker.isEnabled());

        final CheckBox cbEnableTouchScreen = dialog.findViewById(R.id.CBTouchScreen);

        final CheckBox cbShowTouchscreenControls = dialog.findViewById(R.id.CBShowTouchscreenControls);
        cbShowTouchscreenControls.setChecked(inputControlsView.isShowTouchscreenControls());

        dialog.findViewById(R.id.BTSettings).setOnClickListener((v) -> {
            int position = sProfile.getSelectedItemPosition();
            Intent intent = new Intent(this, InputControllerActivity.class);
            intent.putExtra("edit_input_controls", true);
            intent.putExtra("selected_profile_id", position > 0 ? inputControlsManager.getProfiles().get(position - 1).id : 0);
            editInputControlsCallback = () -> {
                hideInputControls();
                inputControlsManager.loadProfiles(true);
                loadProfileSpinner.run();
            };
            startActivityForResult(intent, InputControllerActivity.EDIT_INPUT_CONTROLS_REQUEST_CODE);
        });

        dialog.setOnConfirmCallback(() -> {
            if (termuxActivityListener == null) {
                return;
            }
            xServer.cursorLocker.setEnabled(cbLockCursor.isChecked() ? true : false);
            inputControlsView.setShowTouchscreenControls(cbShowTouchscreenControls.isChecked());
            int position = sProfile.getSelectedItemPosition();
            if (position > 0) {
                if (cbEnableTouchScreen.isChecked()) {
                    touchpadView.setTouchMode(TouchpadView.TouchMode.TOUCH_SCREEN);
                } else {
                    touchpadView.setTouchMode(TouchpadView.TouchMode.TRACK_PAD);
                }
                showInputControls(inputControlsManager.getProfiles().get(position - 1));
            } else {
                hideInputControls();
            }
        });

        dialog.show();
    }

    protected void showInputControls(ControlsProfile controlsProfile) {
        inputControlsView.setVisibility(View.VISIBLE);
        inputControlsView.requestFocus();
        inputControlsView.setProfile(controlsProfile);

        if (profile != null) {
            touchpadView.setSensitivity(profile.getCursorSpeed() * globalCursorSpeed);
        }
        touchpadView.setVisibility(View.VISIBLE);

        inputControlsView.invalidate();
        touchShow = true;
        loriePreferenceFragment.updatePreferencesLayout();
        if (termuxActivityListener!=null){
            termuxActivityListener.onX11PreferenceSwitchChange(false);
        }
    }

    public void hideInputControls() {
        inputControlsView.setShowTouchscreenControls(true);
        inputControlsView.setVisibility(View.GONE);
        inputControlsView.setProfile(null);

        touchpadView.setVisibility(View.GONE);

        inputControlsView.invalidate();
        touchShow = false;
        loriePreferenceFragment.updatePreferencesLayout();
    }
}
