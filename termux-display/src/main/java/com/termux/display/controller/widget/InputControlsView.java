package com.termux.display.controller.widget;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ColorFilter;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Point;
import android.graphics.PointF;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffColorFilter;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;

import com.termux.display.controller.inputcontrols.Binding;
import com.termux.display.controller.inputcontrols.ControlElement;
import com.termux.display.controller.inputcontrols.ControlsProfile;
import com.termux.display.controller.inputcontrols.ExternalController;
import com.termux.display.controller.inputcontrols.ExternalControllerBinding;
import com.termux.display.controller.inputcontrols.GamepadState;
import com.termux.display.controller.math.Mathf;
import com.termux.display.controller.winhandler.WinHandler;
import com.termux.display.controller.xserver.Pointer;
import com.termux.display.LorieView;

import java.io.IOException;
import java.io.InputStream;
import java.util.Timer;
import java.util.TimerTask;

public class InputControlsView extends View {
    public static final float DEFAULT_OVERLAY_OPACITY = 0.4f;
    private boolean editMode = false;
    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Path path = new Path();
    private final ColorFilter colorFilter = new PorterDuffColorFilter(0xffffffff, PorterDuff.Mode.SRC_IN);
    private final Point cursor = new Point();
    private boolean readyToDraw = false;
    private boolean moveCursor = false;
    private int snappingSize;
    private float offsetX;
    private float offsetY;
    private ControlElement selectedElement;
    private ControlsProfile profile;
    private float overlayOpacity = DEFAULT_OVERLAY_OPACITY;
    private LorieView xServer;
    private final Bitmap[] icons = new Bitmap[17];
    private Timer mouseMoveTimer;
    private final PointF mouseMoveOffset = new PointF();
    private boolean showTouchscreenControls = true;

    public View getRootNodeView() {
        return rootNodeView;
    }

    public void setRootNodeView(View rootNodeView) {
        this.rootNodeView = rootNodeView;
    }

    private View rootNodeView;

    public InputControlsView(Context context) {
        super(context);
        setClickable(true);
        setFocusable(true);
        setFocusableInTouchMode(true);
        setBackgroundColor(0x00000000);
        setLayoutParams(new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
    }

    public void setEditMode(boolean editMode) {
        this.editMode = editMode;
    }

    public void setOverlayOpacity(float overlayOpacity) {
        this.overlayOpacity = overlayOpacity;
    }

    public int getSnappingSize() {
        return snappingSize;
    }

    @Override
    protected synchronized void onDraw(Canvas canvas) {
        int width = getWidth();
        int height = getHeight();

        if (width == 0 || height == 0) {
            readyToDraw = false;
            return;
        }
        snappingSize = Math.max(width, height) / 100;

        readyToDraw = true;

        if (editMode) {
            drawGrid(canvas);
            drawCursor(canvas);
        }
        if (profile != null) {
            if (!profile.isElementsLoaded()) {
                profile.loadElements(this);
            }
            if (showTouchscreenControls)
                for (ControlElement element : profile.getElements()) element.draw(canvas);
        }

        super.onDraw(canvas);
    }

    private void drawGrid(Canvas canvas) {
        paint.setStyle(Paint.Style.FILL);
        paint.setStrokeWidth(snappingSize * 0.0625f);
        paint.setColor(0xff000000);
        canvas.drawColor(Color.BLACK);

        paint.setAntiAlias(false);
        paint.setColor(0xff303030);

        int width = getMaxWidth();
        int height = getMaxHeight();

        for (int i = 0; i < width; i += snappingSize) {
            canvas.drawLine(i, 0, i, height, paint);
            canvas.drawLine(0, i, width, i, paint);
        }

        float cx = Mathf.roundTo(width * 0.5f, snappingSize);
        float cy = Mathf.roundTo(height * 0.5f, snappingSize);
        paint.setColor(0xff424242);

        for (int i = 0; i < width; i += snappingSize * 2) {
            canvas.drawLine(cx, i, cx, i + snappingSize, paint);
            canvas.drawLine(i, cy, i + snappingSize, cy, paint);
        }

        paint.setAntiAlias(true);
    }

    private void drawCursor(Canvas canvas) {
        paint.setStyle(Paint.Style.FILL);
        paint.setStrokeWidth(snappingSize * 0.0625f);
        paint.setColor(0xffc62828);

        paint.setAntiAlias(false);
        canvas.drawLine(0, cursor.y, getMaxWidth(), cursor.y, paint);
        canvas.drawLine(cursor.x, 0, cursor.x, getMaxHeight(), paint);

        paint.setAntiAlias(true);
    }

    public synchronized boolean addElement() {
        if (editMode && profile != null) {
            ControlElement element = new ControlElement(this);
            element.setX(cursor.x);
            element.setY(cursor.y);
            profile.addElement(element);
            profile.save();
            selectElement(element);
            return true;
        } else return false;
    }

    public synchronized boolean removeElement() {
        if (editMode && selectedElement != null && profile != null) {
            profile.removeElement(selectedElement);
            selectedElement = null;
            profile.save();
            invalidate();
            return true;
        } else return false;
    }

    public ControlElement getSelectedElement() {
        return selectedElement;
    }

    private synchronized void deselectAllElements() {
        selectedElement = null;
        if (profile != null) {
            for (ControlElement element : profile.getElements()) element.setSelected(false);
        }
    }

    private void selectElement(ControlElement element) {
        deselectAllElements();
        if (element != null) {
            selectedElement = element;
            selectedElement.setSelected(true);
        }
        invalidate();
    }

    public synchronized ControlsProfile getProfile() {
        return profile;
    }

    public synchronized void setProfile(ControlsProfile profile) {
        if (profile != null) {
            this.profile = profile;
            deselectAllElements();
        } else this.profile = null;
    }

    public boolean isShowTouchscreenControls() {
        return showTouchscreenControls;
    }

    public void setShowTouchscreenControls(boolean showTouchscreenControls) {
        this.showTouchscreenControls = showTouchscreenControls;
    }

    public int getPrimaryColor() {
        return Color.argb((int) (overlayOpacity * 255), 255, 255, 255);
    }

    public int getSecondaryColor() {
        return Color.argb((int) (overlayOpacity * 255), 2, 119, 189);
    }

    private synchronized ControlElement intersectElement(float x, float y) {
        if (profile != null) {
            for (ControlElement element : profile.getElements()) {
                if (element.containsPoint(x, y)) return element;
            }
        }
        return null;
    }

    public Paint getPaint() {
        return paint;
    }

    public Path getPath() {
        return path;
    }

    public ColorFilter getColorFilter() {
        return colorFilter;
    }

    public LorieView getXServer() {
        return xServer;
    }

    public void setXServer(LorieView xServer) {
        this.xServer = xServer;
        createMouseMoveTimer();
    }

    public int getMaxWidth() {
        return (int) Mathf.roundTo(getWidth(), snappingSize);
    }

    public int getMaxHeight() {
        return (int) Mathf.roundTo(getHeight(), snappingSize);
    }

    private void createMouseMoveTimer() {
        if (profile != null && mouseMoveTimer == null) {
            final float cursorSpeed = profile.getCursorSpeed();
            mouseMoveTimer = new Timer();
            mouseMoveTimer.schedule(new TimerTask() {
                @Override
                public void run() {
                    xServer.injectPointerMoveDelta((int) (mouseMoveOffset.x * 10 * cursorSpeed), (int) (mouseMoveOffset.y * 10 * cursorSpeed));
                }
            }, 0, 1000 / 60);
        }
    }

    private void processJoystickInput(ExternalController controller) {
        ExternalControllerBinding controllerBinding;
        final int[] axes = {MotionEvent.AXIS_X, MotionEvent.AXIS_Y, MotionEvent.AXIS_Z, MotionEvent.AXIS_RZ, MotionEvent.AXIS_HAT_X, MotionEvent.AXIS_HAT_Y};
        final float[] values = {controller.state.thumbLX, controller.state.thumbLY, controller.state.thumbRX, controller.state.thumbRY, controller.state.getDPadX(), controller.state.getDPadY()};

        for (byte i = 0; i < axes.length; i++) {
            if (Math.abs(values[i]) > ControlElement.STICK_DEAD_ZONE) {
                controllerBinding = controller.getControllerBinding(ExternalControllerBinding.getKeyCodeForAxis(axes[i], Mathf.sign(values[i])));
                if (controllerBinding != null)
                    handleInputEvent(controllerBinding.getBinding(), true, values[i]);
            } else {
                controllerBinding = controller.getControllerBinding(ExternalControllerBinding.getKeyCodeForAxis(axes[i], (byte) 1));
                if (controllerBinding != null)
                    handleInputEvent(controllerBinding.getBinding(), false, values[i]);
                controllerBinding = controller.getControllerBinding(ExternalControllerBinding.getKeyCodeForAxis(axes[i], (byte) -1));
                if (controllerBinding != null)
                    handleInputEvent(controllerBinding.getBinding(), false, values[i]);
            }
        }
    }

    @Override
    public boolean onGenericMotionEvent(MotionEvent event) {
        if (!editMode && profile != null) {
            ExternalController controller = profile.getController(event.getDeviceId());
            if (controller != null && controller.updateStateFromMotionEvent(event)) {
                ExternalControllerBinding controllerBinding;
                controllerBinding = controller.getControllerBinding(KeyEvent.KEYCODE_BUTTON_L2);
                if (controllerBinding != null)
                    handleInputEvent(controllerBinding.getBinding(), controller.state.isPressed(ExternalController.IDX_BUTTON_L2));

                controllerBinding = controller.getControllerBinding(KeyEvent.KEYCODE_BUTTON_R2);
                if (controllerBinding != null)
                    handleInputEvent(controllerBinding.getBinding(), controller.state.isPressed(ExternalController.IDX_BUTTON_R2));

                processJoystickInput(controller);
                return true;
            }
        }
        return super.onGenericMotionEvent(event);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (editMode && readyToDraw) {
//            Log.d("onTouchEvent.inputControllerview.editmode", "<<<<<<<<<<<<<>>>>>>>>>>>>>>>>>>>");
            switch (event.getAction()) {
                case MotionEvent.ACTION_DOWN: {
                    float x = event.getX();
                    float y = event.getY();

                    ControlElement element = intersectElement(x, y);
                    moveCursor = true;
                    if (element != null) {
                        offsetX = x - element.getX();
                        offsetY = y - element.getY();
                        moveCursor = false;
                    }
                    selectElement(element);
                    break;
                }
                case MotionEvent.ACTION_MOVE: {
                    if (selectedElement != null) {
                        selectedElement.setX((int) Mathf.roundTo(event.getX() - offsetX, snappingSize));
                        selectedElement.setY((int) Mathf.roundTo(event.getY() - offsetY, snappingSize));
                        invalidate();
                    }
                    break;
                }
                case MotionEvent.ACTION_UP: {
                    if (selectedElement != null && profile != null) profile.save();
                    if (moveCursor)
                        cursor.set((int) Mathf.roundTo(event.getX(), snappingSize), (int) Mathf.roundTo(event.getY(), snappingSize));
                    invalidate();
                    break;
                }
            }
        }

//        if (!editMode && profile != null) {
////            Log.d("onTouchEvent.inputControllerview.interactivemode", "<<<<<<<<<<<<<>>>>>>>>>>>>>>>>>>>");
////            inputHandler.handleTouchEvent(this,xServer,event);
//            int actionIndex = event.getActionIndex();
//            int pointerId = event.getPointerId(actionIndex);
//            int actionMasked = event.getActionMasked();
//            boolean handled = false;
//
//            switch (actionMasked) {
//                case MotionEvent.ACTION_DOWN:
//                case MotionEvent.ACTION_POINTER_DOWN: {
//                    float x = event.getX(actionIndex);
//                    float y = event.getY(actionIndex);
////                    touchpadView.setPointerButtonLeftEnabled(true);
//                    for (ControlElement element : profile.getElements()) {
//                        if (element.handleTouchDown(pointerId, x, y)) {
////                            Log.d("handleTouchDown",String.valueOf(element.getType()));
//                            handled = true;
//                        }
//                        if (element.getBindingAt(0) == Binding.MOUSE_LEFT_BUTTON) {
//
//                        }
//                    }
//                    if (!handled) {
////                        xServer.sendMouseEvent(0, 0, Pointer.Button.BUTTON_LEFT.code(), true, true);
//                    }
//                    downX = event.getRawX();
//                    downY = event.getRawY();
//                    break;
//                }
//                case MotionEvent.ACTION_MOVE: {
//                    for (byte i = 0, count = (byte) event.getPointerCount(); i < count; i++) {
//                        float x = event.getX(i);
//                        float y = event.getY(i);
////                        Log.d("handleTouchMove","x:"+x+",y:"+y);
//                        handled = false;
//                        for (ControlElement element : profile.getElements()) {
//                            if (element.handleTouchMove(i, event.getRawX() ,event.getRawY())) {
////                                Log.d("handleTouchMove",String.valueOf(element.getType()));
//                                handled = true;
//                            }
//                        }
//                        if (!handled) {
//                        }
//                    }
//                    break;
//                }
//                case MotionEvent.ACTION_UP:
//                case MotionEvent.ACTION_POINTER_UP:
//                case MotionEvent.ACTION_CANCEL:
//                    for (ControlElement element : profile.getElements())
//                        if (element.handleTouchUp(pointerId)) {
////                            Log.d("handleTouchUp",String.valueOf(element.getType()));
//                            handled = true;
//                        }
//                    if (!handled) {
//                        float dx = event.getRawX() - downX;
//                        float dy = event.getRawY() - downY;
//                        if (Math.abs(dx) < 8 && Math.abs(dy) < 8) {
//                            xServer.sendMouseEvent(0, 0, Pointer.Button.BUTTON_LEFT.code(), true, true);
//                            xServer.sendMouseEvent(0, 0, Pointer.Button.BUTTON_LEFT.code(), false, true);
//                        }
//                    }
//                    break;
//            }
//        }
        return false;
    }

    public boolean handleTouchEvent(View view, MotionEvent event) {
        if (!editMode && profile != null) {
            if (view != this) {
                int[] view0Location = new int[2];
                int[] viewLocation = new int[2];

                view.getLocationOnScreen(view0Location);
                this.getLocationOnScreen(viewLocation);

                int offsetX = viewLocation[0] - view0Location[0];
                int offsetY = viewLocation[1] - view0Location[1];

                event.offsetLocation(-offsetX, -offsetY);
                if (!view.isFocused() && event.getAction() == MotionEvent.ACTION_DOWN)
                    view.requestFocus();

                if (event.getAction() == MotionEvent.ACTION_UP)
                    view.requestPointerCapture();
            }
            int actionIndex = event.getActionIndex();
            int pointerId = event.getPointerId(actionIndex);
            int actionMasked = event.getActionMasked();
            boolean handled = false;
//            Log.d("handleTouchEvent",String.valueOf(event.getAction()));
            switch (actionMasked) {
                case MotionEvent.ACTION_DOWN:
                case MotionEvent.ACTION_POINTER_DOWN: {
                    float x = event.getX(actionIndex);
                    float y = event.getY(actionIndex);
//                    touchpadView.setPointerButtonLeftEnabled(true);
                    for (ControlElement element : profile.getElements()) {
                        if (element.handleTouchDown(pointerId, x, y)) {
                            handled = true;
                        }
                        if (element.getBindingAt(0) == Binding.MOUSE_LEFT_BUTTON) {

                        }
                    }
                    if (!handled) {
                    }
                    break;
                }
                case MotionEvent.ACTION_MOVE: {
                    for (byte i = 0, count = (byte) event.getPointerCount(); i < count; i++) {
                        float x = event.getX(i);
                        float y = event.getY(i);
                        handled = false;
                        for (ControlElement element : profile.getElements()) {
                            if (element.handleTouchMove(i, x, y)) {
                                handled = true;
                            }
                        }
                        if (!handled) {
                        }
                    }
                    break;
                }
                case MotionEvent.ACTION_UP:
                case MotionEvent.ACTION_POINTER_UP:
                case MotionEvent.ACTION_CANCEL:
                    for (byte i = 0, count = (byte) event.getPointerCount(); i < count; i++) {
                        float x = event.getX(i);
                        float y = event.getY(i);
                        for (ControlElement element : profile.getElements())
                            if (element.handleTouchUp(pointerId, x, y)) {
                                handled = true;
                            }
                        if (!handled) {
                        }
                    }
                    break;
            }
            return handled;
        }
        return false;
    }

    public void handleInputEvent(Binding binding, boolean isActionDown) {
        handleInputEvent(binding, isActionDown, 0);
    }

    public void handleInputEvent(Binding binding, boolean isActionDown, float offset) {
        if (binding.isGamepad()) {
            WinHandler winHandler = xServer != null ? xServer.getWinHandler() : null;
            GamepadState state = profile.getGamepadState();

            int buttonIdx = binding.ordinal() - Binding.GAMEPAD_BUTTON_A.ordinal();
            if (buttonIdx <= 11) {
                state.setPressed(buttonIdx, isActionDown);
                if (winHandler != null) winHandler.saveGamepadState(state);
            } else if (binding == Binding.GAMEPAD_LEFT_THUMB_UP || binding == Binding.GAMEPAD_LEFT_THUMB_DOWN) {
                state.thumbLY = isActionDown ? offset : 0;
            } else if (binding == Binding.GAMEPAD_LEFT_THUMB_LEFT || binding == Binding.GAMEPAD_LEFT_THUMB_RIGHT) {
                state.thumbLX = isActionDown ? offset : 0;
            } else if (binding == Binding.GAMEPAD_RIGHT_THUMB_UP || binding == Binding.GAMEPAD_RIGHT_THUMB_DOWN) {
                state.thumbRY = isActionDown ? offset : 0;
            } else if (binding == Binding.GAMEPAD_RIGHT_THUMB_LEFT || binding == Binding.GAMEPAD_RIGHT_THUMB_RIGHT) {
                state.thumbRX = isActionDown ? offset : 0;
            } else if (binding == Binding.GAMEPAD_DPAD_UP || binding == Binding.GAMEPAD_DPAD_RIGHT ||
                binding == Binding.GAMEPAD_DPAD_DOWN || binding == Binding.GAMEPAD_DPAD_LEFT) {
                state.dpad[binding.ordinal() - Binding.GAMEPAD_DPAD_UP.ordinal()] = isActionDown;
            }

            if (winHandler != null) {
                ExternalController controller = winHandler.getCurrentController();
                if (controller != null) controller.state.copy(state);
            }
        } else {
            if (binding == Binding.MOUSE_MOVE_LEFT || binding == Binding.MOUSE_MOVE_RIGHT) {
//                Log.d("handleInputEvent","<binding == Binding.MOUSE_MOVE_LEFT || binding == Binding.MOUSE_MOVE_RIGHT>:"+binding.toString());
                mouseMoveOffset.x = isActionDown ? (offset != 0 ? offset : (binding == Binding.MOUSE_MOVE_LEFT ? -1 : 1)) : 0;
                if (isActionDown) createMouseMoveTimer();
            } else if (binding == Binding.MOUSE_MOVE_DOWN || binding == Binding.MOUSE_MOVE_UP) {
//                Log.d("handleInputEvent","<binding == Binding.MOUSE_MOVE_DOWN || binding == Binding.MOUSE_MOVE_UP> "+binding.toString());
                mouseMoveOffset.y = isActionDown ? (offset != 0 ? offset : (binding == Binding.MOUSE_MOVE_UP ? -1 : 1)) : 0;
                if (isActionDown) createMouseMoveTimer();
            } else {
                Pointer.Button pointerButton = binding.getPointerButton();
                if (isActionDown) {
//                    Log.d("handleInputEvent","<isActionDown> "+binding.toString());
                    if (pointerButton != null) {
                        xServer.injectPointerButtonPress(pointerButton);
                    } else {
                        xServer.injectKeyPress(binding.keycode);
                    }
                } else {
//                    Log.d("handleInputEvent","<isActionUp> "+binding.toString());
                    if (pointerButton != null) {
                        xServer.injectPointerButtonRelease(pointerButton);
                    } else {
                        xServer.injectKeyRelease(binding.keycode);
                    }
                }
            }
        }
    }

    public Bitmap getIcon(byte id) {
        if (icons[id] == null) {
            Context context = getContext();
            try (InputStream is = context.getAssets().open("inputcontrols/icons/" + id + ".png")) {
                icons[id] = BitmapFactory.decodeStream(is);
            } catch (IOException e) {
            }
        }
        return icons[id];
    }
}
