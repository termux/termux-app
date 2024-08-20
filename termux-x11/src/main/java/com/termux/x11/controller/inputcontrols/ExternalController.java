package com.termux.x11.controller.inputcontrols;

import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;

import androidx.annotation.Nullable;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;

public class ExternalController {
    public static final byte IDX_BUTTON_A = 0;
    public static final byte IDX_BUTTON_B = 1;
    public static final byte IDX_BUTTON_X = 2;
    public static final byte IDX_BUTTON_Y = 3;
    public static final byte IDX_BUTTON_L1 = 4;
    public static final byte IDX_BUTTON_R1 = 5;
    public static final byte IDX_BUTTON_SELECT = 6;
    public static final byte IDX_BUTTON_START = 7;
    public static final byte IDX_BUTTON_L3 = 8;
    public static final byte IDX_BUTTON_R3 = 9;
    public static final byte IDX_BUTTON_L2 = 10;
    public static final byte IDX_BUTTON_R2 = 11;
    private String name;
    private String id;
    private int deviceId = -1;
    private final ArrayList<ExternalControllerBinding> controllerBindings = new ArrayList<>();
    public final GamepadState state = new GamepadState();

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public String getId() {
        return id;
    }

    public void setId(String id) {
        this.id = id;
    }

    public int getDeviceId() {
        if (this.deviceId == -1) {
            for (int deviceId : InputDevice.getDeviceIds()) {
                InputDevice device = InputDevice.getDevice(deviceId);
                if (device != null && device.getDescriptor().equals(id)) {
                    this.deviceId = deviceId;
                    break;
                }
            }
        }
        return this.deviceId;
    }

    public boolean isConnected() {
        for (int deviceId : InputDevice.getDeviceIds()) {
            InputDevice device = InputDevice.getDevice(deviceId);
            if (device != null && device.getDescriptor().equals(id)) return true;
        }
        return false;
    }

    public ExternalControllerBinding getControllerBinding(int keyCode) {
        for (ExternalControllerBinding controllerBinding : controllerBindings) {
            if (controllerBinding.getKeyCodeForAxis() == keyCode) return controllerBinding;
        }
        return null;
    }

    public ExternalControllerBinding getControllerBindingAt(int index) {
        return controllerBindings.get(index);
    }

    public void addControllerBinding(ExternalControllerBinding controllerBinding) {
        if (getControllerBinding(controllerBinding.getKeyCodeForAxis()) == null) controllerBindings.add(controllerBinding);
    }

    public int getPosition(ExternalControllerBinding controllerBinding) {
        return controllerBindings.indexOf(controllerBinding);
    }

    public void removeControllerBinding(ExternalControllerBinding controllerBinding) {
        controllerBindings.remove(controllerBinding);
    }

    public int getControllerBindingCount() {
        return controllerBindings.size();
    }

    public JSONObject toJSONObject() {
        try {
            if (controllerBindings.isEmpty()) return null;
            JSONObject controllerJSONObject = new JSONObject();
            controllerJSONObject.put("id", id);
            controllerJSONObject.put("name", name);

            JSONArray controllerBindingsJSONArray = new JSONArray();
            for (ExternalControllerBinding controllerBinding : controllerBindings) controllerBindingsJSONArray.put(controllerBinding.toJSONObject());
            controllerJSONObject.put("controllerBindings", controllerBindingsJSONArray);

            return controllerJSONObject;
        }
        catch (JSONException e) {
            return null;
        }
    }

    @Override
    public boolean equals(@Nullable Object obj) {
        return obj instanceof ExternalController ? ((ExternalController)obj).id.equals(this.id) : super.equals(obj);
    }

    private void processJoystickInput(MotionEvent event, int historyPos) {
        state.thumbLX = getCenteredAxis(event, MotionEvent.AXIS_X, historyPos);
        state.thumbLY = getCenteredAxis(event, MotionEvent.AXIS_Y, historyPos);
        state.thumbRX = getCenteredAxis(event, MotionEvent.AXIS_Z, historyPos);
        state.thumbRY = getCenteredAxis(event, MotionEvent.AXIS_RZ, historyPos);

        if (historyPos == -1) {
            float axisX = getCenteredAxis(event, MotionEvent.AXIS_HAT_X, historyPos);
            float axisY = getCenteredAxis(event, MotionEvent.AXIS_HAT_Y, historyPos);

            state.dpad[0] = axisY == -1.0f && Math.abs(state.thumbLY) < ControlElement.STICK_DEAD_ZONE;
            state.dpad[1] = axisX ==  1.0f && Math.abs(state.thumbLX) < ControlElement.STICK_DEAD_ZONE;
            state.dpad[2] = axisY ==  1.0f && Math.abs(state.thumbLY) < ControlElement.STICK_DEAD_ZONE;
            state.dpad[3] = axisX == -1.0f && Math.abs(state.thumbLX) < ControlElement.STICK_DEAD_ZONE;
        }
    }

    private void processTriggerButton(MotionEvent event) {
        state.setPressed(IDX_BUTTON_L2, event.getAxisValue(MotionEvent.AXIS_LTRIGGER) == 1.0f || event.getAxisValue(MotionEvent.AXIS_BRAKE) == 1.0f);
        state.setPressed(IDX_BUTTON_R2, event.getAxisValue(MotionEvent.AXIS_RTRIGGER) == 1.0f || event.getAxisValue(MotionEvent.AXIS_GAS) == 1.0f);
    }

    public boolean updateStateFromMotionEvent(MotionEvent event) {
        if (isJoystickDevice(event)) {
            processTriggerButton(event);
            int historySize = event.getHistorySize();
            for (int i = 0; i < historySize; i++) processJoystickInput(event, i);
            processJoystickInput(event, -1);
            return true;
        }
        return false;
    }

    public boolean updateStateFromKeyEvent(KeyEvent event) {
        boolean pressed = event.getAction() == KeyEvent.ACTION_DOWN;
        int keyCode = event.getKeyCode();
        int buttonIdx = getButtonIdxByKeyCode(keyCode);
        if (buttonIdx != -1) {
            state.setPressed(buttonIdx, pressed);
            return true;
        }

        switch (keyCode) {
            case KeyEvent.KEYCODE_DPAD_UP:
                state.dpad[0] = pressed && Math.abs(state.thumbLY) < ControlElement.STICK_DEAD_ZONE;
                return true;
            case KeyEvent.KEYCODE_DPAD_RIGHT:
                state.dpad[1] = pressed && Math.abs(state.thumbLX) < ControlElement.STICK_DEAD_ZONE;
                return true;
            case KeyEvent.KEYCODE_DPAD_DOWN:
                state.dpad[2] = pressed && Math.abs(state.thumbLY) < ControlElement.STICK_DEAD_ZONE;
                return true;
            case KeyEvent.KEYCODE_DPAD_LEFT:
                state.dpad[3] = pressed && Math.abs(state.thumbLX) < ControlElement.STICK_DEAD_ZONE;
                return true;
        }
        return false;
    }

    public static ArrayList<ExternalController> getControllers() {
        int[] deviceIds = InputDevice.getDeviceIds();
        ArrayList<ExternalController> controllers = new ArrayList<>();
        for (int i = deviceIds.length-1; i >= 0; i--) {
            InputDevice device = InputDevice.getDevice(deviceIds[i]);
            if (isGameController(device)) {
                ExternalController controller = new ExternalController();
                controller.setId(device.getDescriptor());
                controller.setName(device.getName());
                controllers.add(controller);
            }
        }
        return controllers;
    }

    public static ExternalController getController(String id) {
        for (ExternalController controller : getControllers()) if (controller.getId().equals(id)) return controller;
        return null;
    }

    public static ExternalController getController(int deviceId) {
        int[] deviceIds = InputDevice.getDeviceIds();
        for (int i = deviceIds.length-1; i >= 0; i--) {
            if (deviceIds[i] == deviceId || deviceId == 0) {
                InputDevice device = InputDevice.getDevice(deviceIds[i]);
                if (isGameController(device)) {
                    ExternalController controller = new ExternalController();
                    controller.setId(device.getDescriptor());
                    controller.setName(device.getName());
                    controller.deviceId = deviceIds[i];
                    return controller;
                }
            }
        }
        return null;
    }

    public static boolean isGameController(InputDevice device) {
        if (device == null) return false;
        int sources = device.getSources();
        return !device.isVirtual() && ((sources & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD ||
               (sources & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK);
    }

    public static float getCenteredAxis(MotionEvent event, int axis, int historyPos) {
        if (axis == MotionEvent.AXIS_HAT_X || axis == MotionEvent.AXIS_HAT_Y) {
            float value = event.getAxisValue(axis);
            if (Math.abs(value) == 1.0f) return value;
        }
        else {
            InputDevice device = event.getDevice();
            InputDevice.MotionRange range = device.getMotionRange(axis, event.getSource());
            if (range != null) {
                float flat = range.getFlat();
                float value = historyPos < 0 ? event.getAxisValue(axis) : event.getHistoricalAxisValue(axis, historyPos);
                if (Math.abs(value) > flat) return value;
            }
        }
        return 0;
    }

    public static boolean isJoystickDevice(MotionEvent event) {
        return (event.getSource() & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK && event.getAction() == MotionEvent.ACTION_MOVE;
    }

    public static int getButtonIdxByKeyCode(int keyCode) {
        switch (keyCode) {
            case KeyEvent.KEYCODE_BUTTON_A:
                return IDX_BUTTON_A;
            case KeyEvent.KEYCODE_BUTTON_B:
                return IDX_BUTTON_B;
            case KeyEvent.KEYCODE_BUTTON_X:
                return IDX_BUTTON_X;
            case KeyEvent.KEYCODE_BUTTON_Y:
                return IDX_BUTTON_Y;
            case KeyEvent.KEYCODE_BUTTON_L1:
                return IDX_BUTTON_L1;
            case KeyEvent.KEYCODE_BUTTON_R1:
                return IDX_BUTTON_R1;
            case KeyEvent.KEYCODE_BUTTON_SELECT:
                return IDX_BUTTON_SELECT;
            case KeyEvent.KEYCODE_BUTTON_START:
                return IDX_BUTTON_START;
            case KeyEvent.KEYCODE_BUTTON_THUMBL:
                return IDX_BUTTON_L3;
            case KeyEvent.KEYCODE_BUTTON_THUMBR:
                return IDX_BUTTON_R3;
            case KeyEvent.KEYCODE_BUTTON_L2:
                return IDX_BUTTON_L2;
            case KeyEvent.KEYCODE_BUTTON_R2:
                return IDX_BUTTON_R2;
            default:
                return -1;
        }
    }

    public static int getButtonIdxByName(String name) {
        switch (name) {
            case "A":
                return IDX_BUTTON_A;
            case "B":
                return IDX_BUTTON_B;
            case "X":
                return IDX_BUTTON_X;
            case "Y":
                return IDX_BUTTON_Y;
            case "L1":
                return IDX_BUTTON_L1;
            case "R1":
                return IDX_BUTTON_R1;
            case "SELECT":
                return IDX_BUTTON_SELECT;
            case "START":
                return IDX_BUTTON_START;
            case "L3":
                return IDX_BUTTON_L3;
            case "R3":
                return IDX_BUTTON_R3;
            case "L2":
                return IDX_BUTTON_L2;
            case "R2":
                return IDX_BUTTON_R2;
            default:
                return -1;
        }
    }
}
