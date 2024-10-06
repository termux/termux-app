package com.termux.x11.controller;

import android.app.Activity;
import android.content.Intent;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.RadioGroup;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.TextView;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import com.termux.x11.R;
import com.termux.x11.controller.core.AppUtils;
import com.termux.x11.controller.core.FileUtils;
import com.termux.x11.controller.core.UnitUtils;
import com.termux.x11.controller.inputcontrols.Binding;
import com.termux.x11.controller.inputcontrols.ControlElement;
import com.termux.x11.controller.inputcontrols.ControlsProfile;
import com.termux.x11.controller.inputcontrols.InputControlsManager;
import com.termux.x11.controller.math.Mathf;
import com.termux.x11.controller.widget.ColorPickerView;
import com.termux.x11.controller.widget.ImagePickerView;
import com.termux.x11.controller.widget.InputControlsView;
import com.termux.x11.controller.widget.NumberPicker;

import java.io.IOException;
import java.io.InputStream;
import java.util.Arrays;

public class ControlsEditorActivity extends AppCompatActivity implements View.OnClickListener {
    public interface LoadCallBack<T> {
        String call(T t);
    }

    private InputControlsView inputControlsView;
    private ControlsProfile profile;
    protected LoadCallBack<Uri> openFileCallback;

    public void setOpenFileCallback(LoadCallBack<Uri> openFileCallback) {
        this.openFileCallback = openFileCallback;
    }

    @Override
    public void onCreate(Bundle bundle) {
        super.onCreate(bundle);
        AppUtils.hideSystemUI(this);
        setContentView(R.layout.controls_editor_activity);

        inputControlsView = new InputControlsView(this);
        inputControlsView.setEditMode(true);
        inputControlsView.setOverlayOpacity(0.6f);

        profile = InputControlsManager.loadProfile(this, ControlsProfile.getProfileFile(this, getIntent().getIntExtra("profile_id", 0)));
        ((TextView) findViewById(R.id.TVProfileName)).setText(profile.getName());
        inputControlsView.setProfile(profile);

        FrameLayout container = findViewById(R.id.FLContainer);
        container.addView(inputControlsView, 0);

        container.findViewById(R.id.BTAddElement).setOnClickListener(this);
        container.findViewById(R.id.BTRemoveElement).setOnClickListener(this);
        container.findViewById(R.id.BTElementSettings).setOnClickListener(this);
    }

    @Override
    public void onClick(View v) {
        int id = v.getId();
        if (id == R.id.BTAddElement) {
            if (!inputControlsView.addElement()) {
                AppUtils.showToast(this, R.string.no_profile_selected);
            }
        } else if (id == R.id.BTRemoveElement) {
            if (!inputControlsView.removeElement()) {
                AppUtils.showToast(this, R.string.no_control_element_selected);
            }
        } else if (id == R.id.BTElementSettings) {
            ControlElement selectedElement = inputControlsView.getSelectedElement();
            if (selectedElement != null) {
                showControlElementSettings(v);
            } else AppUtils.showToast(this, R.string.no_control_element_selected);
        }
    }

    private void showControlElementSettings(View anchorView) {
        final ControlElement element = inputControlsView.getSelectedElement();
        View view = LayoutInflater.from(this).inflate(R.layout.control_element_settings, null);

        final Runnable updateLayout = () -> {
            ControlElement.Type type = element.getType();
            view.findViewById(R.id.LLShape).setVisibility(View.GONE);
            view.findViewById(R.id.CBToggleSwitch).setVisibility(View.GONE);
            view.findViewById(R.id.LLCustomTextIcon).setVisibility(View.GONE);
            view.findViewById(R.id.LLRangeOptions).setVisibility(View.GONE);

            if (type == ControlElement.Type.BUTTON ||
                type == ControlElement.Type.COMBINE_BUTTON ||
                type == ControlElement.Type.CHEAT_CODE_TEXT) {
                view.findViewById(R.id.LLShape).setVisibility(View.VISIBLE);
                view.findViewById(R.id.CBToggleSwitch).setVisibility(View.VISIBLE);
                view.findViewById(R.id.LLCustomTextIcon).setVisibility(View.VISIBLE);
            } else if (type == ControlElement.Type.RANGE_BUTTON) {
                view.findViewById(R.id.LLRangeOptions).setVisibility(View.VISIBLE);
            }

            loadBindingSpinners(element, view);
        };

        loadTypeSpinner(element, view.findViewById(R.id.SType), updateLayout);
        loadShapeSpinner(element, view.findViewById(R.id.SShape));
        loadRangeSpinner(element, view.findViewById(R.id.SRange));

        RadioGroup rgOrientation = view.findViewById(R.id.RGOrientation);
        rgOrientation.check(element.getOrientation() == 1 ? R.id.RBVertical : R.id.RBHorizontal);
        rgOrientation.setOnCheckedChangeListener((group, checkedId) -> {
            element.setOrientation((byte) (checkedId == R.id.RBVertical ? 1 : 0));
            profile.save();
            inputControlsView.invalidate();
        });

        NumberPicker npColumns = view.findViewById(R.id.NPColumns);
        npColumns.setValue(element.getBindingCount());
        npColumns.setOnValueChangeListener((numberPicker, value) -> {
            element.setBindingCount(value);
            profile.save();
            inputControlsView.invalidate();
        });

        final TextView tvScale = view.findViewById(R.id.TVScale);
        SeekBar sbScale = view.findViewById(R.id.SBScale);
        sbScale.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                tvScale.setText(progress + "%");
                if (fromUser) {
                    progress = (int) Mathf.roundTo(progress, 5);
                    seekBar.setProgress(progress);
                    element.setScale(progress / 100.0f);
                    profile.save();
                    inputControlsView.invalidate();
                }
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
            }
        });
        sbScale.setProgress((int) (element.getScale() * 100));

        CheckBox cbToggleSwitch = view.findViewById(R.id.CBToggleSwitch);
        cbToggleSwitch.setChecked(element.isToggleSwitch());
        cbToggleSwitch.setOnCheckedChangeListener((buttonView, isChecked) -> {
            element.setToggleSwitch(isChecked);
            profile.save();
        });

        final EditText etCustomText = view.findViewById(R.id.ETCustomText);
        etCustomText.setText(element.getText());
        final LinearLayout llIconList = view.findViewById(R.id.LLIconList);
        loadIcons(llIconList, element.getIconId());
        loadCustomIconType(element, view);

        updateLayout.run();

        PopupWindow popupWindow = AppUtils.showPopupWindow(anchorView, view, 340, 0);
        popupWindow.setOnDismissListener(() -> {
            String text = etCustomText.getText().toString().trim();
            byte iconId = 0;
            for (int i = 0; i < llIconList.getChildCount(); i++) {
                View child = llIconList.getChildAt(i);
                if (child.isSelected()) {
                    iconId = (byte) child.getTag();
                    break;
                }
            }

            element.setText(text);
            element.setIconId(iconId);
            profile.save();
            inputControlsView.invalidate();
        });
    }

    private void loadCustomIconType(ControlElement element, View view) {
        Spinner iconTypeSpinner = view.findViewById(R.id.SControllerIconType);
        ColorPickerView colorPicker = view.findViewById(R.id.CPVControllerBackgroundColor);
        ImagePickerView iconPicker = view.findViewById(R.id.IPVControllerBackgroundImage);
        iconTypeSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                colorPicker.setVisibility(View.GONE);
                iconPicker.setVisibility(View.GONE);
                if (position == 0) {
                    if (element.getCustomIconId() != null && !element.getCustomIconId().isEmpty()) {
                        iconPicker.setImageId(element.getCustomIconId());
                    }
                    iconPicker.setVisibility(View.VISIBLE);
                } else {
                    if (element.getCustomIconId() != null && element.getBackgroundColor() >= 0) {
                        colorPicker.setColor(element.getBackgroundColor());
                    }
                    colorPicker.setVisibility(View.VISIBLE);
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                if (element.getCustomIconId() != null && element.getBackgroundColor() >= 0) {
                    colorPicker.setColor(element.getBackgroundColor());
                }
                colorPicker.setVisibility(View.VISIBLE);
            }
        });
    }

    private void loadTypeSpinner(final ControlElement element, Spinner spinner, Runnable callback) {
        spinner.setAdapter(new ArrayAdapter<>(this, android.R.layout.simple_spinner_dropdown_item, ControlElement.Type.names()));
        spinner.setSelection(element.getType().ordinal(), false);
        spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                element.setType(ControlElement.Type.values()[position]);
                profile.save();
                callback.run();
                inputControlsView.invalidate();
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
            }
        });
    }

    private void loadShapeSpinner(final ControlElement element, Spinner spinner) {
        spinner.setAdapter(new ArrayAdapter<>(this, android.R.layout.simple_spinner_dropdown_item, ControlElement.Shape.names()));
        spinner.setSelection(element.getShape().ordinal(), false);
        spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                element.setShape(ControlElement.Shape.values()[position]);
                profile.save();
                inputControlsView.invalidate();
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
            }
        });
    }

    private void loadBindingSpinners(ControlElement element, View view) {
        LinearLayout container = view.findViewById(R.id.LLBindings);
        container.removeAllViews();

        ControlElement.Type type = element.getType();
        if (type == ControlElement.Type.BUTTON) {
            loadBindingSpinner(element, container, 0, R.string.binding);
        } else if (type == ControlElement.Type.D_PAD || type == ControlElement.Type.STICK) {
            loadBindingSpinner(element, container, 0, R.string.binding_up);
            loadBindingSpinner(element, container, 1, R.string.binding_right);
            loadBindingSpinner(element, container, 2, R.string.binding_down);
            loadBindingSpinner(element, container, 3, R.string.binding_left);
        } else if (type == ControlElement.Type.COMBINE_BUTTON) {
            LinearLayout addButtonLayout = new LinearLayout(this);
            LinearLayout.LayoutParams addButtonLayoutParam = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
            addButtonLayout.setLayoutParams(addButtonLayoutParam);
            addButtonLayout.setOrientation(LinearLayout.VERTICAL);
            Button btn = new Button(this);
            btn.setText(R.string.add_button);
            btn.setTextColor(getResources().getColor(R.color.colorBlack, null));
            btn.setOnClickListener(v -> {
                if (addButtonLayout.getChildCount() < 5) {
                    loadCombineBindingSpinner(element, addButtonLayout, addButtonLayout.getChildCount() - 1);
                } else {
                    AppUtils.showToast(this, R.string.too_much_key);
                }
            });
            addButtonLayout.addView(btn);
            for (int idx = 0; idx < element.getBindingCount(); idx++) {
                if (element.getBindingAt(idx) != Binding.NONE) {
                    loadCombineBindingSpinner(element, addButtonLayout, idx);
                }
            }
            container.addView(addButtonLayout);
        } else if (type == ControlElement.Type.CHEAT_CODE_TEXT) {
            LinearLayout cheatCodeLayout = new LinearLayout(this);
            LinearLayout.LayoutParams cheatCodeLayoutParam = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
            cheatCodeLayout.setLayoutParams(cheatCodeLayoutParam);
            cheatCodeLayout.setOrientation(LinearLayout.VERTICAL);
            loadCheatCodeTextEditor(element, cheatCodeLayout);
            container.addView(cheatCodeLayout);
        }
    }

    private void loadBindingSpinner(final ControlElement element, LinearLayout container, final int index, int titleResId) {
        View view = LayoutInflater.from(this).inflate(R.layout.binding_field, container, false);
        ((TextView) view.findViewById(R.id.TVTitle)).setText(titleResId);
        final Spinner sBindingType = view.findViewById(R.id.SBindingType);
        final Spinner sBinding = view.findViewById(R.id.SBinding);

        Runnable update = () -> {
            String[] bindingEntries = null;
            switch (sBindingType.getSelectedItemPosition()) {
                case 0:
                    bindingEntries = Binding.keyboardBindingLabels();
                    break;
                case 1:
                    bindingEntries = Binding.mouseBindingLabels();
                    break;
                case 2:
                    bindingEntries = Binding.gamepadBindingLabels();
                    break;
            }

            sBinding.setAdapter(new ArrayAdapter<>(this, android.R.layout.simple_spinner_dropdown_item, bindingEntries));
            AppUtils.setSpinnerSelectionFromValue(sBinding, element.getBindingAt(index).toString());
        };

        sBindingType.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                update.run();
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
            }
        });

        Binding selectedBinding = element.getBindingAt(index);
        if (selectedBinding.isKeyboard()) {
            sBindingType.setSelection(0, false);
        } else if (selectedBinding.isMouse()) {
            sBindingType.setSelection(1, false);
        } else if (selectedBinding.isGamepad()) {
            sBindingType.setSelection(2, false);
        }

        sBinding.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                Binding binding = Binding.NONE;
                switch (sBindingType.getSelectedItemPosition()) {
                    case 0:
                        binding = Binding.keyboardBindingValues()[position];
                        break;
                    case 1:
                        binding = Binding.mouseBindingValues()[position];
                        break;
                    case 2:
                        binding = Binding.gamepadBindingValues()[position];
                        break;
                }

                if (binding != element.getBindingAt(index)) {
                    element.setBindingAt(index, binding);
                    profile.save();
                    inputControlsView.invalidate();
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
            }
        });

        update.run();
        container.addView(view);
    }

    private void loadCombineBindingSpinner(final ControlElement element, LinearLayout container, final int index) {
        View view = LayoutInflater.from(this).inflate(R.layout.binding_combine_field, container, false);
        final Spinner sBindingType = view.findViewById(R.id.SCBBindingType);
        final Spinner sBinding = view.findViewById(R.id.SCBBinding);
        final ImageButton deleteButton = view.findViewById(R.id.IBTNDelKeyBinding);
        deleteButton.setOnClickListener(v -> {
            container.removeView(view);
        });

        Runnable update = () -> {
            String[] bindingEntries = null;
            switch (sBindingType.getSelectedItemPosition()) {
                case 0:
                    bindingEntries = Binding.keyboardBindingLabels();
                    break;
                case 1:
                    bindingEntries = Binding.mouseBindingLabels();
                    break;
                case 2:
                    bindingEntries = Binding.gamepadBindingLabels();
                    break;
            }

            sBinding.setAdapter(new ArrayAdapter<>(this, android.R.layout.simple_spinner_dropdown_item, bindingEntries));
            AppUtils.setSpinnerSelectionFromValue(sBinding, element.getBindingAt(index).toString());
        };

        sBindingType.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                update.run();
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
            }
        });

        Binding selectedBinding = element.getBindingAt(index);
        if (selectedBinding.isKeyboard()) {
            sBindingType.setSelection(0, false);
        } else if (selectedBinding.isMouse()) {
            sBindingType.setSelection(1, false);
        } else if (selectedBinding.isGamepad()) {
            sBindingType.setSelection(2, false);
        }

        sBinding.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                Binding binding = Binding.NONE;
                switch (sBindingType.getSelectedItemPosition()) {
                    case 0:
                        binding = Binding.keyboardBindingValues()[position];
                        break;
                    case 1:
                        binding = Binding.mouseBindingValues()[position];
                        break;
                    case 2:
                        binding = Binding.gamepadBindingValues()[position];
                        break;
                }

                if (binding != element.getBindingAt(index)) {
                    element.setBindingAt(index, binding);
                    profile.save();
                    inputControlsView.invalidate();
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
            }
        });

        update.run();
        container.addView(view);
    }

    private void loadCheatCodeTextEditor(final ControlElement element, LinearLayout container) {
        TextView tv = new TextView(this);
        tv.setText(R.string.cheat_code);
        container.addView(tv);

        View view = LayoutInflater.from(this).inflate(R.layout.cheat_code_itm, container, false);
        final EditText ed = view.findViewById(R.id.EDCCT);
        final ImageButton setBtn = view.findViewById(R.id.IBTNSET);
        String cct = element.getCheatCodeText();
        ed.setText(cct);
        setBtn.setOnClickListener(v -> {
            String cheatCode = ed.getText().toString();
            if (!cheatCode.contentEquals(cct)) {
                element.setCheatCodeText(cheatCode);
                profile.save();
                inputControlsView.invalidate();
            }
        });
        container.addView(view);
    }

    private void loadRangeSpinner(final ControlElement element, Spinner spinner) {
        spinner.setAdapter(new ArrayAdapter<>(this, android.R.layout.simple_spinner_dropdown_item, ControlElement.Range.names()));
        spinner.setSelection(element.getRange().ordinal(), false);
        spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                element.setRange(ControlElement.Range.values()[position]);
                profile.save();
                inputControlsView.invalidate();
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
            }
        });
    }

    private void loadIcons(final LinearLayout parent, byte selectedId) {
        byte[] iconIds = new byte[0];
        try {
            String[] filenames = getAssets().list("inputcontrols/icons/");
            iconIds = new byte[filenames.length];
            for (int i = 0; i < filenames.length; i++) {
                iconIds[i] = Byte.parseByte(FileUtils.getBasename(filenames[i]));
            }
        } catch (IOException e) {
        }

        Arrays.sort(iconIds);

        int size = (int) UnitUtils.dpToPx(40);
        int margin = (int) UnitUtils.dpToPx(2);
        int padding = (int) UnitUtils.dpToPx(4);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(size, size);
        params.setMargins(margin, 0, margin, 0);

        for (final byte id : iconIds) {
            ImageView imageView = new ImageView(this);
            imageView.setLayoutParams(params);
            imageView.setPadding(padding, padding, padding, padding);
            imageView.setBackgroundResource(R.drawable.icon_background);
            imageView.setTag(id);
            imageView.setSelected(id == selectedId);
            imageView.setOnClickListener((v) -> {
                for (int i = 0; i < parent.getChildCount(); i++)
                    parent.getChildAt(i).setSelected(false);
                imageView.setSelected(true);
            });

            try (InputStream is = getAssets().open("inputcontrols/icons/" + id + ".png")) {
                imageView.setImageBitmap(BitmapFactory.decodeStream(is));
            } catch (IOException e) {
            }

            parent.addView(imageView);
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == getResources().getInteger(R.integer.load_button_icon_code) && resultCode == Activity.RESULT_OK) {
            if (openFileCallback != null) {
                String id = openFileCallback.call(data.getData());
                if (id != null && inputControlsView.getSelectedElement() != null) {
                    inputControlsView.getSelectedElement().setCustomIconId(id);
                    inputControlsView.invalidate();
                }
                openFileCallback = null;
            }
        }
    }

    public boolean unLoadCustomIcon() {
        if (inputControlsView != null &&
            inputControlsView.getSelectedElement() != null) {
            String iconId = inputControlsView.getSelectedElement().getCustomIconId();
            inputControlsView.counterMapDecrease(iconId);
            inputControlsView.getSelectedElement().setCustomIconId(null);
            return inputControlsView.counterMapZero(iconId);
        }
        return false;
    }

    public void setButtonColor(int color) {
        if (inputControlsView != null &&
            inputControlsView.getSelectedElement() != null) {
            inputControlsView.getSelectedElement().setBackgroundColor(color);
            profile.save();
            inputControlsView.invalidate();
        }
    }

    public void setCustomButtonIcon(String id) {
        if (inputControlsView != null &&
            inputControlsView.getSelectedElement() != null) {
            String iconId = inputControlsView.getSelectedElement().getCustomIconId();
            if (iconId!=null&&iconId.equals(id)) {
                return;
            }
            if (id == null) {
                inputControlsView.counterMapDecrease(iconId);
            }
            inputControlsView.getSelectedElement().setCustomIconId(id);
        }
    }
}
