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
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.database.ContentObserver;
import android.graphics.Typeface;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.preference.ListPreference;
import androidx.preference.Preference;

import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;
import android.preference.PreferenceManager;

import androidx.annotation.Nullable;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.preference.PreferenceDataStore;
import androidx.preference.PreferenceFragmentCompat;

import androidx.preference.Preference.OnPreferenceChangeListener;
import androidx.preference.PreferenceScreen;
import androidx.preference.SeekBarPreference;

import android.provider.Settings;
import android.text.method.LinkMovementMethod;
import android.util.Log;
import android.util.TypedValue;
import android.view.Display;
import android.view.InputDevice;
import android.view.MenuItem;
import android.view.View;
import android.view.WindowManager;
import android.widget.ArrayAdapter;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

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
import com.termux.x11.utils.TermuxX11ExtraKeys;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Objects;
import java.util.Scanner;
import java.util.Set;
import java.util.function.Consumer;
import java.util.regex.PatternSyntaxException;

@SuppressWarnings("deprecation")
public class LoriePreferences extends AppCompatActivity implements PreferenceFragmentCompat.OnPreferenceStartFragmentCallback {
    static final String ACTION_PREFERENCES_CHANGED = "com.termux.x11.ACTION_PREFERENCES_CHANGED";
    public static Prefs prefs = null;

    public static int OPEN_FILE_REQUEST_CODE = 102;
    static final String SHOW_IME_WITH_HARD_KEYBOARD = "show_ime_with_hard_keyboard";
    protected LoriePreferenceFragment loriePreferenceFragment;
    protected LorieView xServer;
    protected static boolean touchShow = false;
    protected int orientation = ActivityInfo.SCREEN_ORIENTATION_PORTRAIT;

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

    private   int id_preference_view;
    public void setPreferenceViewId(int viewId){
        id_preference_view = viewId;
    }


    private final BroadcastReceiver receiver = new BroadcastReceiver() {
        @SuppressLint("UnspecifiedRegisterReceiverFlag")
        @Override
        public void onReceive(Context context, Intent intent) {
            if (ACTION_PREFERENCES_CHANGED.equals(intent.getAction()) &&
                intent.getBooleanExtra("fromBroadcast", false))
                updatePreferencesLayout();
        }
    };

    private final ContentObserver accessibilityObserver = new ContentObserver(null) {
        private final Runnable updateLayout = () -> updatePreferencesLayout();

        @Override
        public void onChange(boolean selfChange) {
            handler.removeCallbacks(updateLayout);
            handler.postDelayed(updateLayout, 200);
        }

        @Override
        public boolean deliverSelfNotifications() {
            return true;
        }
    };

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus)
            updatePreferencesLayout();
    }

    protected void updatePreferencesLayout() {
        getSupportFragmentManager().getFragments().forEach(fragment -> {
            if (fragment instanceof LoriePreferenceFragment)
                ((LoriePreferenceFragment) fragment).updatePreferencesLayout();
        });
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        prefs = new Prefs(this);
//        loriePreferenceFragment = new LoriePreferenceFragment(null);
//        getSupportFragmentManager().beginTransaction().replace(android.R.id.content, new LoriePreferenceFragment(null)).commit();

        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            actionBar.setDisplayHomeAsUpEnabled(true);
            actionBar.setHomeButtonEnabled(true);
        }

        Uri ENABLED_ACCESSIBILITY_SERVICES = Settings.Secure.getUriFor(Settings.Secure.ENABLED_ACCESSIBILITY_SERVICES);
        Uri ACCESSIBILITY_ENABLED = Settings.Secure.getUriFor(Settings.Secure.ACCESSIBILITY_ENABLED);

        getContentResolver().registerContentObserver(ENABLED_ACCESSIBILITY_SERVICES, true, accessibilityObserver);
        getContentResolver().registerContentObserver(ACCESSIBILITY_ENABLED, true, accessibilityObserver);
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
            if (getSupportFragmentManager().getBackStackEntryCount() == 0)
                finish();
            else
                onBackPressed();

            return true;
        }

        return super.onOptionsItemSelected(item);
    }

    protected void showFragment(PreferenceFragmentCompat fragment) {
        getSupportFragmentManager().beginTransaction()
//            .setCustomAnimations(R.anim.slide_in_right, R.anim.slide_out_left, R.anim.slide_in_left, R.anim.slide_out_right)
            .replace(id_preference_view, fragment)
            .addToBackStack(null)
            .commit();
    }

    @Override
    public boolean onPreferenceStartFragment(@NonNull PreferenceFragmentCompat caller, @NonNull Preference pref) {
        final LoriePreferenceFragment fragment = new LoriePreferenceFragment(pref.getFragment());
        if (fragment.loriePreferences==null){
            fragment.loriePreferences=this;
        }
        fragment.setTargetFragment(caller, 0);
        showFragment(fragment);
        return true;
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

    public static class LoriePreferenceFragment extends PreferenceFragmentCompat implements OnPreferenceChangeListener {
        private LoriePreferences loriePreferences;

        private final Runnable updateLayout = this::updatePreferencesLayout;
        private static final Method onSetInitialValue;
        static {
            try {
                //noinspection JavaReflectionMemberAccess
                onSetInitialValue = Preference.class.getDeclaredMethod("onSetInitialValue", boolean.class, Object.class);
                onSetInitialValue.setAccessible(true);
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
            ActionBar actionBar = ((LoriePreferences) getActivity()).getSupportActionBar();
            if(actionBar!=null){
                actionBar.setTitle(getPreferenceScreen().getTitle());
            }
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

        /** @noinspection DataFlowIssue*/
        @Override @SuppressLint("ApplySharedPref")
        public void onCreatePreferences(@Nullable Bundle savedInstanceState, @Nullable String rootKey) {
            getPreferenceManager().setPreferenceDataStore(prefs);

            if ((Integer.parseInt(prefs.touchMode.get()) - 1) > 2)
                prefs.touchMode.put("1");

            setPreferencesFromResource(R.xml.preferences, root == null ? "main" : root);

            int id;
            PreferenceScreen screen = getPreferenceScreen();
            if ((id = findId(screen.getKey())) != 0)
                getPreferenceScreen().setTitle(getResources().getString(id));
            for (int i=0; i<getPreferenceScreen().getPreferenceCount(); i++) {
                Preference p = screen.getPreference(i);
                p.setOnPreferenceChangeListener(this);
                p.setPreferenceDataStore(prefs);

                if ((id = findId(p.getKey())) != 0)
                    p.setTitle(getResources().getString(id));

                if ((id = findId(p.getKey() + "_summary")) != 0)
                    p.setSummary(getResources().getString(id));

                if (p instanceof ListPreference) {
                    ListPreference list = (ListPreference) p;
                    list.setEntries(prefs.keys.get(p.getKey()).asList().getEntries());
                    list.setEntryValues(prefs.keys.get(p.getKey()).asList().getValues());
                    list.setSummaryProvider(ListPreference.SimpleSummaryProvider.getInstance());
                }
            }

            with("showAdditionalKbd", p -> p.setLayoutResource(R.layout.display_preference));
            with("version", p -> p.setSummary(BuildConfig.VERSION_NAME));

            setSummary("displayStretch", R.string.pref_summary_requiresExactOrCustom);
            setSummary("adjustResolution", R.string.pref_summary_requiresExactOrCustom);
            setSummary("pauseKeyInterceptingWithEsc", R.string.pref_summary_requiresIntercepting);
            setSummary("scaleTouchpad", R.string.pref_summary_requiresTrackpadAndNative);

            if (!SamsungDexUtils.available())
                setVisible("dexMetaKeyCapture", false);

            if (Build.VERSION.SDK_INT < Build.VERSION_CODES.P)
                setVisible("hideCutout", false);

            boolean stylusAvailable = Arrays.stream(InputDevice.getDeviceIds())
                .mapToObj(InputDevice::getDevice)
                .filter(Objects::nonNull)
                .anyMatch(d -> d.supportsSource(InputDevice.SOURCE_STYLUS));

            setVisible("showStylusClickOverride", stylusAvailable);
            setVisible("stylusIsMouse", stylusAvailable);
            setVisible("stylusButtonContactModifierMode", stylusAvailable);

            setNoActionOptionText(findPreference("volumeDownAction"), "android volume control");
            setNoActionOptionText(findPreference("volumeUpAction"), "android volume control");
            setNoActionOptionText(findPreference("mediaKeysAction"), "android media control");
        }

        private void setSummary(CharSequence key, int disabled) {
            Preference pref = findPreference(key);
            if (pref != null)
                pref.setSummaryProvider(new Preference.SummaryProvider<>() {
                    @Nullable @Override public CharSequence provideSummary(@NonNull Preference p) {
                        return p.isEnabled() ? null : getResources().getString(disabled);
                    }
                });
        }

        private void setVisible(CharSequence key, boolean value) {
            Preference p = findPreference(key);
            if (p != null)
                p.setVisible(value);
        }

        private void setEnabled(CharSequence key, boolean value) {
            Preference p = findPreference(key);
            if (p != null)
                p.setEnabled(value);
        }

        @SuppressWarnings("ConstantConditions")
        void updatePreferencesLayout() {
            if (getContext() == null)
                return;

            for (String key : prefs.keys.keySet()) {
                Preference p = findPreference(key);
                if (p != null)
                    onSetInitialValue(p);
            }

            String displayResMode = prefs.displayResolutionMode.get();
            setVisible("displayScale", displayResMode.contentEquals("scaled"));
            setVisible("displayResolutionExact", displayResMode.contentEquals("exact"));
            setVisible("displayResolutionCustom", displayResMode.contentEquals("custom"));

            setEnabled("dexMetaKeyCapture", !prefs.enableAccessibilityServiceAutomatically.get());
            setEnabled("enableAccessibilityServiceAutomatically", !prefs.dexMetaKeyCapture.get());
            setEnabled("pauseKeyInterceptingWithEsc", prefs.dexMetaKeyCapture.get() ||
                prefs.enableAccessibilityServiceAutomatically.get() ||
                KeyInterceptor.isLaunched());
            setEnabled("enableAccessibilityServiceAutomatically", prefs.enableAccessibilityServiceAutomatically.get() || KeyInterceptor.isLaunched());
            setEnabled("filterOutWinkey", prefs.enableAccessibilityServiceAutomatically.get() || KeyInterceptor.isLaunched());

            boolean displayStretchEnabled = "exact".contentEquals(prefs.displayResolutionMode.get()) || "custom".contentEquals(prefs.displayResolutionMode.get());
            setEnabled("displayStretch", displayStretchEnabled);
            setEnabled("adjustResolution", displayStretchEnabled);

            setEnabled("scaleTouchpad", "1".equals(prefs.touchMode.get()) && !"native".equals(prefs.displayResolutionMode.get()));
            setEnabled("showMouseHelper", "1".equals(prefs.touchMode.get()));

            boolean requestNotificationPermissionVisible =
                Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU
                    && ContextCompat.checkSelfPermission(requireContext(), POST_NOTIFICATIONS) == PERMISSION_DENIED;
            setVisible("requestNotificationPermission", requestNotificationPermissionVisible);
        }

        /** @noinspection SameParameterValue*/
        private void setNoActionOptionText(Preference preference, CharSequence text) {
            if (preference == null)
                return;
            ListPreference p = (ListPreference) preference;
            CharSequence[] options = p.getEntries();
            for (int i=0; i<options.length; i++) {
                if ("no action".contentEquals(options[i]))
                    options[i] = "no action (" + text + ")";
            }
        }

        @Override
        public void onCreate(final Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);

            updatePreferencesLayout();
        }

        @Override
        public boolean onPreferenceTreeClick(@NonNull Preference p) {
            if (p.getKey() == null)
                return super.onPreferenceTreeClick(p);

            if ("version".contentEquals(p.getKey())) {
                Context ctx = getContext();
                if (ctx != null) {
                    ((ClipboardManager) ctx.getSystemService(Context.CLIPBOARD_SERVICE))
                        .setPrimaryClip(ClipData.newPlainText(p.getSummary(), p.getSummary()));
                    Toast.makeText(ctx, "Copied to clipboard", Toast.LENGTH_SHORT).show();
                }
            }
            if("return".contentEquals(p.getKey())){
                loriePreferences.getSupportFragmentManager().popBackStack();
            }

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU && "requestNotificationPermission".contentEquals(p.getKey())) {
                ActivityCompat.requestPermissions(requireActivity(), new String[]{POST_NOTIFICATIONS}, 101);
                return true;
            }

            updatePreferencesLayout();
            return super.onPreferenceTreeClick(p);
        }

        @SuppressLint("ApplySharedPref")
        @Override
        public boolean onPreferenceChange(Preference preference, Object newValue) {
            String key = preference.getKey();
            Log.e("Preferences", "changed preference: " + key);
            handler.removeCallbacks(updateLayout);
            handler.postDelayed(updateLayout, 50);

            if ("displayScale".contentEquals(key)) {
                int scale = (Integer) newValue;
                if (scale % 10 != 0) {
                    scale = Math.round( ( (float) scale ) / 10 ) * 10;
                    ((SeekBarPreference) preference).setValue(scale);
                    return false;
                }
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

            if ("showAdditionalKbd".contentEquals(key) && (Boolean) newValue)
                prefs.additionalKbdVisible.put(true);

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

            requireContext().sendBroadcast(new Intent(ACTION_PREFERENCES_CHANGED) {{
                putExtra("key", key);
                putExtra("fromBroadcast", true);
                setPackage("com.termux.x11");
            }});

            return true;
        }

        @Override
        public void onDisplayPreferenceDialog(@NonNull Preference preference) {
            if ("extra_keys_config".contentEquals(preference.getKey())) {
                @SuppressLint("InflateParams")
                View view = getLayoutInflater().inflate(R.layout.extra_keys_config, null, false);
                EditText config = view.findViewById(R.id.extra_keys_config);
                config.setTypeface(Typeface.MONOSPACE);
                config.setText(prefs.extra_keys_config.get());
                TextView desc = view.findViewById(R.id.extra_keys_config_description);
                desc.setLinksClickable(true);
                desc.setText(R.string.extra_keys_config_desc);
                desc.setMovementMethod(LinkMovementMethod.getInstance());
                new android.app.AlertDialog.Builder(getActivity())
                    .setView(view)
                    .setTitle("Extra keys config")
                    .setPositiveButton("OK",
                        (dialog, whichButton) -> {
                            String text = config.getText().toString();
                            prefs.extra_keys_config.put(!text.isEmpty() ? text : TermuxX11ExtraKeys.DEFAULT_IVALUE_EXTRA_KEYS);
                        }
                    )
                    .setNeutralButton("Reset",
                        (dialog, whichButton) -> prefs.extra_keys_config.put(TermuxX11ExtraKeys.DEFAULT_IVALUE_EXTRA_KEYS))
                    .setNegativeButton("Cancel", (dialog, whichButton) -> dialog.dismiss())
                    .create()
                    .show();
            } else super.onDisplayPreferenceDialog(preference);
        }
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
//    public static Handler handler = new Handler();

    public void onClick(View view) {
        showFragment(new LoriePreferenceFragment("ekbar"));
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
    private void callProgressManager() {
        if (termuxActivityListener != null) {
            termuxActivityListener.showProcessManager();
        }
    }

    private void openSoftKeyBoard() {
        termuxActivityListener.openSoftwareKeyboard();
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
