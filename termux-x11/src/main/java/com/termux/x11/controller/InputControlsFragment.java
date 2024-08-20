package com.termux.x11.controller;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.ColorStateList;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.preference.PreferenceManager;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.PopupMenu;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import androidx.core.widget.ImageViewCompat;
import androidx.fragment.app.Fragment;

import com.termux.x11.MainActivity;
import com.termux.x11.R;
import com.termux.x11.controller.contentdialog.ContentDialog;
import com.termux.x11.controller.core.AppUtils;
import com.termux.x11.controller.core.Callback;
import com.termux.x11.controller.core.FileUtils;
import com.termux.x11.controller.core.HttpUtils;
import com.termux.x11.controller.inputcontrols.ControlsProfile;
import com.termux.x11.controller.inputcontrols.ExternalController;
import com.termux.x11.controller.inputcontrols.InputControlsManager;
import com.termux.x11.controller.math.Mathf;
import com.termux.x11.controller.widget.InputControlsView;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.util.ArrayList;
import java.util.concurrent.atomic.AtomicInteger;

public class InputControlsFragment extends Fragment {
    private static final String INPUT_CONTROLS_URL = "https://raw.githubusercontent.com/brunodev85/winlator/main/input_controls/%s";
    private InputControlsManager manager;
    private ControlsProfile currentProfile;
    private Runnable updateLayout;
    private Callback<ControlsProfile> importProfileCallback;
    private final int selectedProfileId;

    public InputControlsFragment(int selectedProfileId) {
        this.selectedProfileId = selectedProfileId;
    }
    public InputControlsFragment() {
        selectedProfileId =0;
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setHasOptionsMenu(false);
        manager = new InputControlsManager(getContext());
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
//        ((AppCompatActivity)getActivity()).getSupportActionBar().setTitle(R.string.input_controls);
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        if (requestCode == MainActivity.OPEN_FILE_REQUEST_CODE && resultCode == Activity.RESULT_OK) {
            try {
                ControlsProfile importedProfile = manager.importProfile(new JSONObject(FileUtils.readString(getContext(), data.getData())));
                if (importProfileCallback != null) importProfileCallback.call(importedProfile);
            }
            catch (Exception e) {
                AppUtils.showToast(getContext(), R.string.unable_to_import_profile);
            }
            importProfileCallback = null;
        }
    }

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.input_controls_fragment, container, false);
        final Context context = getContext();
        final SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(context);

        currentProfile = selectedProfileId > 0 ? manager.getProfile(selectedProfileId) : null;

        final Spinner sProfile = view.findViewById(R.id.SProfile);
        loadProfileSpinner(sProfile);

        final TextView tvCursorSpeed = view.findViewById(R.id.TVCursorSpeed);
        final SeekBar sbCursorSpeed = view.findViewById(R.id.SBCursorSpeed);
        sbCursorSpeed.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                tvCursorSpeed.setText(progress+"%");
                if (currentProfile != null) {
                    currentProfile.setCursorSpeed(progress / 100.0f);
                    currentProfile.save();
                }
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {}

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {}
        });

        updateLayout = () -> {
            if (currentProfile != null) {
                sbCursorSpeed.setProgress((int)(currentProfile.getCursorSpeed() * 100));
            }
            else sbCursorSpeed.setProgress(100);
            loadExternalControllers(view);
        };

        updateLayout.run();

        final TextView tvUiOpacity = view.findViewById(R.id.TVUiOpacity);
        SeekBar sbUiOpacity = view.findViewById(R.id.SBOverlayOpacity);
        sbUiOpacity.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                tvUiOpacity.setText(progress+"%");
                if (fromUser) {
                    progress = (int)Mathf.roundTo(progress, 5);
                    seekBar.setProgress(progress);
                    preferences.edit().putFloat("overlay_opacity", progress / 100.0f).apply();
                }
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {}

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {}
        });
        sbUiOpacity.setProgress((int)(preferences.getFloat("overlay_opacity", InputControlsView.DEFAULT_OVERLAY_OPACITY) * 100));

        view.findViewById(R.id.BTAddProfile).setOnClickListener((v) -> ContentDialog.prompt(context, R.string.profile_name, null, (name) -> {
            currentProfile = manager.createProfile(name);
            loadProfileSpinner(sProfile);
            updateLayout.run();
        }));

        view.findViewById(R.id.BTEditProfile).setOnClickListener((v) -> {
            if (currentProfile != null) {
                ContentDialog.prompt(context, R.string.profile_name, currentProfile.getName(), (name) -> {
                    currentProfile.setName(name);
                    currentProfile.save();
                    loadProfileSpinner(sProfile);
                });
            }
            else AppUtils.showToast(context, R.string.no_profile_selected);
        });

        view.findViewById(R.id.BTDuplicateProfile).setOnClickListener((v) -> {
            if (currentProfile != null) {
                ContentDialog.confirm(context, R.string.do_you_want_to_duplicate_this_profile, () -> {
                    currentProfile = manager.duplicateProfile(currentProfile);
                    loadProfileSpinner(sProfile);
                    updateLayout.run();
                });
            }
            else AppUtils.showToast(context, R.string.no_profile_selected);
        });

        view.findViewById(R.id.BTRemoveProfile).setOnClickListener((v) -> {
            if (currentProfile != null) {
                ContentDialog.confirm(context, R.string.do_you_want_to_remove_this_profile, () -> {
                    manager.removeProfile(currentProfile);
                    currentProfile = null;
                    loadProfileSpinner(sProfile);
                    updateLayout.run();
                });
            }
            else AppUtils.showToast(context, R.string.no_profile_selected);
        });

        view.findViewById(R.id.BTImportProfile).setOnClickListener((v) -> {
            PopupMenu popupMenu = new PopupMenu(context, v);
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) popupMenu.setForceShowIcon(true);
            popupMenu.inflate(R.menu.open_file_popup_menu);
            popupMenu.setOnMenuItemClickListener((menuItem) -> {
                int itemId = menuItem.getItemId();
                if (itemId == R.id.open_file) {
                    openProfileFile(sProfile);
                }
                else if (itemId == R.id.download_file) {
                    downloadProfileList(sProfile);
                }
                return true;
            });
            popupMenu.show();
        });

        view.findViewById(R.id.BTExportProfile).setOnClickListener((v) -> {
            if (currentProfile != null) {
                File exportedFile = manager.exportProfile(currentProfile);
                if (exportedFile != null) {
                    String path = exportedFile.getPath().substring(exportedFile.getPath().indexOf(Environment.DIRECTORY_DOWNLOADS));
                    AppUtils.showToast(context, context.getString(R.string.profile_exported_to)+" "+path);
                }
            }
            else AppUtils.showToast(context, R.string.no_profile_selected);
        });

        view.findViewById(R.id.BTControlsEditor).setOnClickListener((v) -> {
            if (currentProfile != null) {
                Intent intent = new Intent(context, ControlsEditorActivity.class);
                intent.putExtra("profile_id", currentProfile.id);
                startActivity(intent);
            }
            else AppUtils.showToast(context, R.string.no_profile_selected);
        });

        return view;
    }

    private void openProfileFile(Spinner sProfile) {
        importProfileCallback = (importedProfile) -> {
            currentProfile = importedProfile;
            loadProfileSpinner(sProfile);
            updateLayout.run();
        };
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        getActivity().startActivityFromFragment(this, intent, MainActivity.OPEN_FILE_REQUEST_CODE);
    }

    private void downloadSelectedProfiles(final Spinner sProfile, String[] items, final ArrayList<Integer> positions) {
        final MainActivity activity = (MainActivity)getActivity();
        activity.getPreloaderDialog().show(R.string.downloading_file);
        currentProfile = null;
        final AtomicInteger processedItemCount = new AtomicInteger();

        for (int position : positions) {
            HttpUtils.download(String.format(INPUT_CONTROLS_URL, items[position]), (content) -> {
                try {
                    if (content != null) manager.importProfile(new JSONObject(content));
                }
                catch (JSONException e) {}
                if (processedItemCount.incrementAndGet() == positions.size()) {
                    activity.runOnUiThread(() -> {
                        activity.getPreloaderDialog().close();
                        loadProfileSpinner(sProfile);
                        updateLayout.run();
                    });
                }
            });
        }
    }

    private void downloadProfileList(final Spinner sProfile) {
        final MainActivity activity = (MainActivity)getActivity();
        activity.getPreloaderDialog().show(R.string.loading);
        HttpUtils.download(String.format(INPUT_CONTROLS_URL, "index.txt"), (content) -> activity.runOnUiThread(() -> {
            activity.getPreloaderDialog().close();
            if (content != null) {
                final String[] items = content.split("\n");
                ContentDialog.showMultipleChoiceList(activity, R.string.import_profile, items, (positions) -> {
                    if (!positions.isEmpty()) {
                        ContentDialog.confirm(activity, R.string.do_you_want_to_download_the_selected_profiles, () -> downloadSelectedProfiles(sProfile, items, positions));
                    }
                });
            }
            else AppUtils.showToast(activity, R.string.unable_to_load_profile_list);
        }));
    }

    @Override
    public void onStart() {
        super.onStart();
        if (updateLayout != null) updateLayout.run();
    }

    private void loadProfileSpinner(Spinner spinner) {
        final ArrayList<ControlsProfile> profiles = manager.getProfiles();
        ArrayList<String> values = new ArrayList<>();
        values.add("-- "+getString(R.string.select_profile)+" --");

        int selectedPosition = 0;
        for (int i = 0; i < profiles.size(); i++) {
            ControlsProfile profile = profiles.get(i);
            if (profile == currentProfile) selectedPosition = i + 1;
            values.add(profile.getName());
        }

        spinner.setAdapter(new ArrayAdapter<>(getContext(), android.R.layout.simple_spinner_dropdown_item, values));
        spinner.setSelection(selectedPosition, false);
        spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                currentProfile = position > 0 ? profiles.get(position - 1) : null;
                updateLayout.run();
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {}
        });
    }

    private void loadExternalControllers(final View view) {
        LinearLayout container = view.findViewById(R.id.LLExternalControllers);
        container.removeAllViews();
        Context context = getContext();
        LayoutInflater inflater = LayoutInflater.from(context);
        ArrayList<ExternalController> connectedControllers = ExternalController.getControllers();

        ArrayList<ExternalController> controllers = currentProfile != null ? currentProfile.loadControllers() : new ArrayList<>();
        for (ExternalController controller : connectedControllers) {
            if (!controllers.contains(controller)) controllers.add(controller);
        }

        if (!controllers.isEmpty()) {
            view.findViewById(R.id.TVEmptyText).setVisibility(View.GONE);
            String bindingsText = context.getString(R.string.bindings);
            for (final ExternalController controller : controllers) {
                View itemView = inflater.inflate(R.layout.external_controller_list_item, container, false);
                ((TextView)itemView.findViewById(R.id.TVTitle)).setText(controller.getName());

                int controllerBindingCount = controller.getControllerBindingCount();
                ((TextView)itemView.findViewById(R.id.TVSubtitle)).setText(controllerBindingCount+" "+bindingsText);

                ImageView imageView = itemView.findViewById(R.id.ImageView);
                int tintColor = controller.isConnected() ? ContextCompat.getColor(context, R.color.colorAccent) : 0xffe57373;
                ImageViewCompat.setImageTintList(imageView, ColorStateList.valueOf(tintColor));

                if (controllerBindingCount > 0) {
                    ImageButton removeButton = itemView.findViewById(R.id.BTRemove);
                    removeButton.setVisibility(View.VISIBLE);
                    removeButton.setOnClickListener((v) -> ContentDialog.confirm(getContext(), R.string.do_you_want_to_remove_this_controller, () -> {
                        currentProfile.removeController(controller);
                        currentProfile.save();
                        loadExternalControllers(view);
                    }));
                }

                itemView.setOnClickListener((v) -> {
                    if (currentProfile != null) {
                        Intent intent = new Intent(getContext(), ExternalControllerBindingsActivity.class);
                        intent.putExtra("profile_id", currentProfile.id);
                        intent.putExtra("controller_id", controller.getId());
                        startActivity(intent);
                    }
                    else AppUtils.showToast(getContext(), R.string.no_profile_selected);
                });

                container.addView(itemView);
            }
        }
        else view.findViewById(R.id.TVEmptyText).setVisibility(View.VISIBLE);
    }
}
