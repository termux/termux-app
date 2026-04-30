
package com.termux.view

import android.annotation.SuppressLint
import android.annotation.TargetApi
import android.app.Activity
import android.content.ClipData
import android.content.ClipboardManager
import android.content.Context
import android.graphics.Canvas
import android.graphics.Typeface
import android.os.Build
import android.os.Handler
import android.os.Looper
import android.os.SystemClock
import android.text.Editable
import android.text.InputType
import android.text.TextUtils
import android.util.AttributeSet
import android.view.*
import android.view.accessibility.AccessibilityManager
import android.view.autofill.AutofillManager
import android.view.autofill.AutofillValue
import android.view.inputmethod.BaseInputConnection
import android.view.inputmethod.EditorInfo
import android.view.inputmethod.InputConnection
import android.widget.Scroller
import androidx.annotation.RequiresApi
import com.termux.terminal.KeyHandler
import com.termux.terminal.TerminalEmulator
import com.termux.terminal.TerminalSession
import com.termux.view.textselection.TextSelectionCursorController
import kotlin.math.abs
import kotlin.math.max
import kotlin.math.min
import kotlin.math.roundToInt

/** View displaying and interacting with a [TerminalSession]. */
class TerminalView(context: Context, attributes: AttributeSet?) : View(context, attributes) {

    var termSession: TerminalSession? = null
        private set

    var emulator: TerminalEmulator? = null
        private set

    var renderer: TerminalRenderer? = null
        private set

    var client: TerminalViewClient? = null

    private var textSelectionCursorController: TextSelectionCursorController? = null
    private var terminalCursorBlinkerHandler: Handler? = null
    private var terminalCursorBlinkerRunnable: TerminalCursorBlinkerRunnable? = null
    private var terminalCursorBlinkerRate = 0

    var topRow = 0
    private val defaultSelectors = intArrayOf(-1, -1, -1, -1)
    private var scaleFactor = 1.0f
    private val gestureRecognizer: GestureAndScaleRecognizer
    private val scroller: Scroller = Scroller(context)

    private var mouseScrollStartX = -1
    private var mouseScrollStartY = -1
    private var mouseStartDownTime: Long = -1
    private var scrollRemainder = 0.0f
    private var combiningAccent = 0

    @RequiresApi(Build.VERSION_CODES.O)
    private var autoFillType = AUTOFILL_TYPE_NONE

    @RequiresApi(Build.VERSION_CODES.O)
    private var autoFillImportance = IMPORTANT_FOR_AUTOFILL_NO

    private var autoFillHints = emptyArray<String>()
    private val accessibilityEnabled: Boolean

    private val showFloatingToolbarRunnable = Runnable {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            textSelectionActionMode?.hide(0)
        }
    }

    init {
        gestureRecognizer = GestureAndScaleRecognizer(context, object : GestureAndScaleRecognizer.Listener {
            var scrolledWithFinger = false

            override fun onUp(event: MotionEvent): Boolean {
                scrollRemainder = 0.0f
                val emu = emulator
                if (emu != null && emu.isMouseTrackingActive && !event.isFromSource(InputDevice.SOURCE_MOUSE) && !isSelectingText && !scrolledWithFinger) {
                    sendMouseEventCode(event, TerminalEmulator.MOUSE_LEFT_BUTTON, true)
                    sendMouseEventCode(event, TerminalEmulator.MOUSE_LEFT_BUTTON, false)
                    return true
                }
                scrolledWithFinger = false
                return false
            }

            override fun onSingleTapUp(event: MotionEvent): Boolean {
                if (emulator == null) return true
                if (isSelectingText) {
                    stopTextSelectionMode()
                    return true
                }
                requestFocus()
                client?.onSingleTapUp(event)
                return true
            }

            override fun onScroll(e: MotionEvent, distanceX: float, distanceY: float): Boolean {
                val emu = emulator ?: return true
                if (emu.isMouseTrackingActive && e.isFromSource(InputDevice.SOURCE_MOUSE)) {
                    sendMouseEventCode(e, TerminalEmulator.MOUSE_LEFT_BUTTON_MOVED, true)
                } else {
                    scrolledWithFinger = true
                    var dY = distanceY + scrollRemainder
                    val deltaRows = (dY / (renderer?.mFontLineSpacing ?: 1f)).toInt()
                    scrollRemainder = dY - deltaRows * (renderer?.mFontLineSpacing ?: 1f)
                    doScroll(e, deltaRows)
                }
                return true
            }

            override fun onScale(focusX: float, focusY: float, scale: float): Boolean {
                if (emulator == null || isSelectingText) return true
                scaleFactor *= scale
                client?.let { scaleFactor = it.onScale(scaleFactor) }
                return true
            }

            override fun onFling(e2: MotionEvent, velocityX: float, velocityY: float): Boolean {
                val emu = emulator ?: return true
                if (!scroller.isFinished) return true

                val mouseTrackingAtStart = emu.isMouseTrackingActive
                val scale = 0.25f
                if (mouseTrackingAtStart) {
                    scroller.fling(0, 0, 0, -(velocityY * scale).toInt(), 0, 0, -emu.mRows / 2, emu.mRows / 2)
                } else {
                    scroller.fling(0, topRow, 0, -(velocityY * scale).toInt(), 0, 0, -emu.screen.activeTranscriptRows, 0)
                }

                post(object : Runnable {
                    private var lastY = 0
                    override fun run() {
                        if (mouseTrackingAtStart != emulator?.isMouseTrackingActive) {
                            scroller.abortAnimation()
                            return
                        }
                        if (scroller.isFinished) return
                        val more = scroller.computeScrollOffset()
                        val newY = scroller.currY
                        val diff = if (mouseTrackingAtStart) newY - lastY else newY - topRow
                        doScroll(e2, diff)
                        lastY = newY
                        if (more) post(this)
                    }
                })
                return true
            }

            override fun onDown(x: float, y: float): Boolean = false

            override fun onDoubleTap(event: MotionEvent): Boolean = false

            override fun onLongPress(event: MotionEvent) {
                if (gestureRecognizer.isInProgress) return
                if (client?.onLongPress(event) == true) return
                if (!isSelectingText) {
                    performHapticFeedback(HapticFeedbackConstants.LONG_PRESS)
                    startTextSelectionMode(event)
                }
            }
        })

        val am = context.getSystemService(Context.ACCESSIBILITY_SERVICE) as AccessibilityManager
        accessibilityEnabled = am.isEnabled
    }

    fun attachSession(session: TerminalSession): Boolean {
        if (session == termSession) return false
        topRow = 0
        termSession = session
        emulator = null
        combiningAccent = 0
        updateSize()
        isVerticalScrollBarEnabled = true
        return true
    }

    override fun onCreateInputConnection(outAttrs: EditorInfo): InputConnection? {
        val currentClient = client ?: return null
        if (currentClient.isTerminalViewSelected) {
            if (currentClient.shouldEnforceCharBasedInput()) {
                outAttrs.inputType = InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD or InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS
            } else {
                outAttrs.inputType = InputType.TYPE_NULL
            }
        } else {
            outAttrs.inputType = InputType.TYPE_CLASS_TEXT or InputType.TYPE_TEXT_VARIATION_NORMAL
        }
        outAttrs.imeOptions = EditorInfo.IME_FLAG_NO_FULLSCREEN

        return object : BaseInputConnection(this, true) {
            override fun finishComposingText(): Boolean {
                if (terminalViewKeyLoggingEnabled) client?.logInfo(LOG_TAG, "IME: finishComposingText()")
                super.finishComposingText()
                sendTextToTerminal(editable)
                editable.clear()
                return true
            }

            override fun commitText(text: CharSequence, newCursorPosition: Int): Boolean {
                if (terminalViewKeyLoggingEnabled) client?.logInfo(LOG_TAG, "IME: commitText(\"$text\", $newCursorPosition)")
                super.commitText(text, newCursorPosition)
                if (emulator == null) return true
                sendTextToTerminal(editable)
                editable.clear()
                return true
            }

            override fun deleteSurroundingText(leftLength: Int, rightLength: Int): Boolean {
                if (terminalViewKeyLoggingEnabled) client?.logInfo(LOG_TAG, "IME: deleteSurroundingText($leftLength, $rightLength)")
                val deleteKey = KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_DEL)
                repeat(leftLength) { sendKeyEvent(deleteKey) }
                return super.deleteSurroundingText(leftLength, rightLength)
            }

            private fun sendTextToTerminal(text: CharSequence) {
                stopTextSelectionMode()
                var i = 0
                while (i < text.length) {
                    val firstChar = text[i]
                    val codePoint = if (Character.isHighSurrogate(firstChar)) {
                        if (++i < text.length) Character.toCodePoint(firstChar, text[i])
                        else TerminalEmulator.UNICODE_REPLACEMENT_CHAR
                    } else firstChar.toInt()

                    var finalCodePoint = codePoint
                    if (client?.readShiftKey() == true) finalCodePoint = Character.toUpperCase(finalCodePoint)

                    var ctrlHeld = false
                    if (finalCodePoint <= 31 && finalCodePoint != 27) {
                        if (finalCodePoint == '\n'.toInt()) finalCodePoint = '\r'.toInt()
                        ctrlHeld = true
                        finalCodePoint = when (finalCodePoint) {
                            31 -> '_'.toInt()
                            30 -> '^'.toInt()
                            29 -> ']'.toInt()
                            28 -> '\\'.toInt()
                            else -> finalCodePoint + 96
                        }
                    }
                    inputCodePoint(KEY_EVENT_SOURCE_SOFT_KEYBOARD, finalCodePoint, ctrlHeld, false)
                    i++
                }
            }
        }
    }

    override fun computeVerticalScrollRange(): Int = emulator?.screen?.activeRows ?: 1
    override fun computeVerticalScrollExtent(): Int = emulator?.mRows ?: 1
    override fun computeVerticalScrollOffset(): Int = emulator?.let { it.screen.activeRows + topRow - it.mRows } ?: 1

    fun onScreenUpdated(skipScrolling: Boolean = false) {
        val emu = emulator ?: return
        var shouldSkip = skipScrolling
        val rowsInHistory = emu.screen.activeTranscriptRows
        if (topRow < -rowsInHistory) topRow = -rowsInHistory

        if (isSelectingText || emu.isAutoScrollDisabled) {
            val rowShift = emu.scrollCounter
            if (-topRow + rowShift > rowsInHistory) {
                if (isSelectingText) stopTextSelectionMode()
                if (emu.isAutoScrollDisabled) {
                    topRow = -rowsInHistory
                    shouldSkip = true
                }
            } else {
                shouldSkip = true
                topRow -= rowShift
                decrementYTextSelectionCursors(rowShift)
            }
        }

        if (!shouldSkip && topRow != 0) {
            if (topRow < -3) awakenScrollBars()
            topRow = 0
        }
        emu.clearScrollCounter()
        invalidate()
        if (accessibilityEnabled) contentDescription = getText()
    }

    fun setTextSize(textSize: Int) {
        renderer = TerminalRenderer(textSize, renderer?.mTypeface ?: Typeface.MONOSPACE)
        updateSize()
    }

    fun setTypeface(newTypeface: Typeface) {
        renderer = TerminalRenderer(renderer?.mTextSize ?: 12, newTypeface)
        updateSize()
        invalidate()
    }

    override fun onCheckIsTextEditor(): Boolean = true
    override fun isOpaque(): Boolean = true

    fun getColumnAndRow(event: MotionEvent, relativeToScroll: Boolean): IntArray {
        val r = renderer ?: return intArrayOf(0, 0)
        val column = (event.x / r.mFontWidth).toInt()
        var row = ((event.y - r.mFontLineSpacingAndAscent) / r.mFontLineSpacing).toInt()
        if (relativeToScroll) row += topRow
        return intArrayOf(column, row)
    }

    private fun sendMouseEventCode(e: MotionEvent, button: Int, pressed: Boolean) {
        val emu = emulator ?: return
        val coords = getColumnAndRow(e, false)
        var x = coords[0] + 1
        var y = coords[1] + 1
        if (pressed && (button == TerminalEmulator.MOUSE_WHEELDOWN_BUTTON || button == TerminalEmulator.MOUSE_WHEELUP_BUTTON)) {
            if (mouseStartDownTime == e.downTime) {
                x = mouseScrollStartX
                y = mouseScrollStartY
            } else {
                mouseStartDownTime = e.downTime
                mouseScrollStartX = x
                mouseScrollStartY = y
            }
        }
        emu.sendMouseEvent(button, x, y, pressed)
    }

    private fun doScroll(event: MotionEvent, rowsDown: Int) {
        val emu = emulator ?: return
        val up = rowsDown < 0
        val amount = abs(rowsDown)
        repeat(amount) {
            when {
                emu.isMouseTrackingActive -> sendMouseEventCode(event, if (up) TerminalEmulator.MOUSE_WHEELUP_BUTTON else TerminalEmulator.MOUSE_WHEELDOWN_BUTTON, true)
                emu.isAlternateBufferActive -> handleKeyCode(if (up) KeyEvent.KEYCODE_DPAD_UP else KeyEvent.KEYCODE_DPAD_DOWN, 0)
                else -> {
                    topRow = min(0, max(-emu.screen.activeTranscriptRows, topRow + if (up) -1 else 1))
                    if (!awakenScrollBars()) invalidate()
                }
            }
        }
    }

    override fun onGenericMotionEvent(event: MotionEvent): Boolean {
        if (emulator != null && event.isFromSource(InputDevice.SOURCE_MOUSE) && event.action == MotionEvent.ACTION_SCROLL) {
            val up = event.getAxisValue(MotionEvent.AXIS_VSCROLL) > 0.0f
            doScroll(event, if (up) -3 else 3)
            return true
        }
        return false
    }

    @SuppressLint("ClickableViewAccessibility")
    override fun onTouchEvent(event: MotionEvent): Boolean {
        val emu = emulator ?: return true
        val action = event.action
        if (isSelectingText) {
            updateFloatingToolbarVisibility(event)
            gestureRecognizer.onTouchEvent(event)
            return true
        } else if (event.isFromSource(InputDevice.SOURCE_MOUSE)) {
            when {
                event.isButtonPressed(MotionEvent.BUTTON_SECONDARY) -> {
                    if (action == MotionEvent.ACTION_DOWN) showContextMenu()
                    return true
                }
                event.isButtonPressed(MotionEvent.BUTTON_TERTIARY) -> {
                    val cb = context.getSystemService(Context.CLIPBOARD_SERVICE) as ClipboardManager
                    cb.primaryClip?.getItemAt(0)?.coerceToText(context)?.let {
                        if (!TextUtils.isEmpty(it)) emu.paste(it.toString())
                    }
                }
                emu.isMouseTrackingActive -> {
                    when (event.action) {
                        MotionEvent.ACTION_DOWN, MotionEvent.ACTION_UP -> sendMouseEventCode(event, TerminalEmulator.MOUSE_LEFT_BUTTON, event.action == MotionEvent.ACTION_DOWN)
                        MotionEvent.ACTION_MOVE -> sendMouseEventCode(event, TerminalEmulator.MOUSE_LEFT_BUTTON_MOVED, true)
                    }
                }
            }
        }
        gestureRecognizer.onTouchEvent(event)
        return true
    }

    override fun onKeyPreIme(keyCode: Int, event: KeyEvent): Boolean {
        if (terminalViewKeyLoggingEnabled) client?.logInfo(LOG_TAG, "onKeyPreIme(keyCode=$keyCode, event=$event)")
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            cancelRequestAutoFill()
            if (isSelectingText) {
                stopTextSelectionMode()
                return true
            } else if (client?.shouldBackButtonBeMappedToEscape() == true) {
                return when (event.action) {
                    KeyEvent.ACTION_DOWN -> onKeyDown(keyCode, event)
                    KeyEvent.ACTION_UP -> onKeyUp(keyCode, event)
                    else -> super.onKeyPreIme(keyCode, event)
                }
            }
        } else if (client?.shouldUseCtrlSpaceWorkaround() == true && keyCode == KeyEvent.KEYCODE_SPACE && event.isCtrlPressed) {
            return onKeyDown(keyCode, event)
        }
        return super.onKeyPreIme(keyCode, event)
    }

    override fun onKeyDown(keyCode: Int, event: KeyEvent): Boolean {
        if (terminalViewKeyLoggingEnabled) client?.logInfo(LOG_TAG, "onKeyDown(keyCode=$keyCode, isSystem()=${event.isSystem}, event=$event)")
        val emu = emulator ?: return true
        if (isSelectingText) stopTextSelectionMode()
        if (client?.onKeyDown(keyCode, event, termSession) == true) {
            invalidate()
            return true
        } else if (event.isSystem && (client?.shouldBackButtonBeMappedToEscape() == false || keyCode != KeyEvent.KEYCODE_BACK)) {
            return super.onKeyDown(keyCode, event)
        } else if (event.action == KeyEvent.ACTION_MULTIPLE && keyCode == KeyEvent.KEYCODE_UNKNOWN) {
            termSession?.write(event.characters)
            return true
        } else if (keyCode == KeyEvent.KEYCODE_LANGUAGE_SWITCH) {
            return super.onKeyDown(keyCode, event)
        }

        val metaState = event.metaState
        val controlDown = event.isCtrlPressed || client?.readControlKey() == true
        val leftAltDown = (metaState and KeyEvent.META_ALT_LEFT_ON != 0) || client?.readAltKey() == true
        val shiftDown = event.isShiftPressed || client?.readShiftKey() == true
        val rightAltDownFromEvent = (metaState and KeyEvent.META_ALT_RIGHT_ON != 0)

        var keyMod = 0
        if (controlDown) keyMod = keyMod or KeyHandler.KEYMOD_CTRL
        if (event.isAltPressed || leftAltDown) keyMod = keyMod or KeyHandler.KEYMOD_ALT
        if (shiftDown) keyMod = keyMod or KeyHandler.KEYMOD_SHIFT
        if (event.isNumLockOn) keyMod = keyMod or KeyHandler.KEYMOD_NUM_LOCK

        if (!event.isFunctionPressed && handleKeyCode(keyCode, keyMod)) {
            if (terminalViewKeyLoggingEnabled) client?.logInfo(LOG_TAG, "handleKeyCode() took key event")
            return true
        }

        var bitsToClear = KeyEvent.META_CTRL_MASK
        if (!rightAltDownFromEvent) bitsToClear = bitsToClear or (KeyEvent.META_ALT_ON or KeyEvent.META_ALT_LEFT_ON)
        
        var effectiveMetaState = event.metaState and bitsToClear.inv()
        if (shiftDown) effectiveMetaState = effectiveMetaState or (KeyEvent.META_SHIFT_ON or KeyEvent.META_SHIFT_LEFT_ON)
        if (client?.readFnKey() == true) effectiveMetaState = effectiveMetaState or KeyEvent.META_FUNCTION_ON

        var result = event.getUnicodeChar(effectiveMetaState)
        if (result == 0) return false

        val oldCombiningAccent = combiningAccent
        if ((result and KeyCharacterMap.COMBINING_ACCENT) != 0) {
            if (combiningAccent != 0) inputCodePoint(event.deviceId, combiningAccent, controlDown, leftAltDown)
            combiningAccent = result and KeyCharacterMap.COMBINING_ACCENT_MASK
        } else {
            if (combiningAccent != 0) {
                val combinedChar = KeyCharacterMap.getDeadChar(combiningAccent, result)
                if (combinedChar > 0) result = combinedChar
                combiningAccent = 0
            }
            inputCodePoint(event.deviceId, result, controlDown, leftAltDown)
        }
        if (combiningAccent != oldCombiningAccent) invalidate()
        return true
    }

    fun inputCodePoint(eventSource: Int, codePoint: Int, controlDownFromEvent: Boolean, leftAltDownFromEvent: Boolean) {
        val session = termSession ?: return
        emulator?.setCursorBlinkState(true)
        val controlDown = controlDownFromEvent || client?.readControlKey() == true
        val altDown = leftAltDownFromEvent || client?.readAltKey() == true

        if (client?.onCodePoint(codePoint, controlDown, session) == true) return

        var cp = codePoint
        if (controlDown) {
            cp = when (cp) {
                in 'a'.toInt()..'z'.toInt() -> cp - 'a'.toInt() + 1
                in 'A'.toInt()..'Z'.toInt() -> cp - 'A'.toInt() + 1
                ' '.toInt(), '2'.toInt() -> 0
                '['.toInt(), '3'.toInt() -> 27
                '\\'.toInt(), '4'.toInt() -> 28
                ']'.toInt(), '5'.toInt() -> 29
                '^'.toInt(), '6'.toInt() -> 30
                '_'.toInt(), '7'.toInt(), '/'.toInt() -> 31
                '8'.toInt() -> 127
                else -> cp
            }
        }

        if (cp > -1) {
            if (eventSource > KEY_EVENT_SOURCE_SOFT_KEYBOARD) {
                cp = when (cp) {
                    0x02DC -> 0x007E
                    0x02CB -> 0x0060
                    0x02C6 -> 0x005E
                    else -> cp
                }
            }
            session.writeCodePoint(altDown, cp)
        }
    }

    fun handleKeyCode(keyCode: Int, keyMod: Int): Boolean {
        emulator?.setCursorBlinkState(true)
        if (handleKeyCodeAction(keyCode, keyMod)) return true
        val emu = emulator ?: return false
        val code = KeyHandler.getCode(keyCode, keyMod, emu.isCursorKeysApplicationMode, emu.isKeypadApplicationMode) ?: return false
        termSession?.write(code)
        return true
    }

    private fun handleKeyCodeAction(keyCode: Int, keyMod: Int): Boolean {
        val shiftDown = (keyMod and KeyHandler.KEYMOD_SHIFT) != 0
        if (shiftDown && (keyCode == KeyEvent.KEYCODE_PAGE_UP || keyCode == KeyEvent.KEYCODE_PAGE_DOWN)) {
            val time = SystemClock.uptimeMillis()
            val ev = MotionEvent.obtain(time, time, MotionEvent.ACTION_DOWN, 0f, 0f, 0)
            doScroll(ev, if (keyCode == KeyEvent.KEYCODE_PAGE_UP) -(emulator?.mRows ?: 0) else (emulator?.mRows ?: 0))
            ev.recycle()
            return true
        }
        return false
    }

    override fun onKeyUp(keyCode: Int, event: KeyEvent): Boolean {
        if (emulator == null && keyCode != KeyEvent.KEYCODE_BACK) return true
        if (client?.onKeyUp(keyCode, event) == true) {
            invalidate()
            return true
        }
        return if (event.isSystem) super.onKeyUp(keyCode, event) else true
    }

    override fun onSizeChanged(w: Int, h: Int, oldw: Int, oldh: Int) {
        updateSize()
    }

    fun updateSize() {
        val r = renderer ?: return
        val session = termSession ?: return
        val viewWidth = width
        val viewHeight = height
        if (viewWidth == 0 || viewHeight == 0) return

        val newColumns = max(4, (viewWidth / r.mFontWidth).toInt())
        val newRows = max(4, (viewHeight - r.mFontLineSpacingAndAscent).toInt() / r.mFontLineSpacing.toInt())

        if (emulator == null || (newColumns != emulator?.mColumns || newRows != emulator?.mRows)) {
            session.updateSize(newColumns, newRows, r.fontWidth.toInt(), r.fontLineSpacing.toInt())
            emulator = session.emulator
            client?.onEmulatorSet()
            terminalCursorBlinkerRunnable?.setEmulator(emulator)
            topRow = 0
            scrollTo(0, 0)
            invalidate()
        }
    }

    override fun onDraw(canvas: Canvas) {
        val emu = emulator
        val r = renderer
        if (emu == null || r == null) {
            canvas.drawColor(-0x1000000)
        } else {
            val sel = defaultSelectors.copyOf()
            textSelectionCursorController?.getSelectors(sel)
            r.render(emu, canvas, topRow, sel[0], sel[1], sel[2], sel[3])
            textSelectionCursorController?.render()
        }
    }

    private fun getText(): CharSequence? = emulator?.screen?.getSelectedText(0, topRow, emulator?.mColumns ?: 0, topRow + (emulator?.mRows ?: 0))

    fun getCursorX(x: float): Int = (x / (renderer?.mFontWidth ?: 1f)).toInt()
    fun getCursorY(y: float): Int = (((y - 40) / (renderer?.mFontLineSpacing ?: 1f)) + topRow).toInt()
    fun getPointX(cx: Int): Int = (min(cx, emulator?.mColumns ?: 0) * (renderer?.mFontWidth ?: 1f)).roundToInt()
    fun getPointY(cy: Int): Int = ((cy - topRow) * (renderer?.mFontLineSpacing ?: 1f)).roundToInt()

    @RequiresApi(Build.VERSION_CODES.O)
    override fun autofill(value: AutofillValue) {
        if (value.isText) termSession?.write(value.textValue.toString())
        resetAutoFill()
    }

    @RequiresApi(Build.VERSION_CODES.O)
    override fun getAutofillType(): Int = autoFillType

    @RequiresApi(Build.VERSION_CODES.O)
    override fun getAutofillHints(): Array<String> = autoFillHints

    @RequiresApi(Build.VERSION_CODES.O)
    override fun getAutofillValue(): AutofillValue = AutofillValue.forText("")

    @RequiresApi(Build.VERSION_CODES.O)
    override fun getImportantForAutofill(): Int = autoFillImportance

    @RequiresApi(Build.VERSION_CODES.O)
    private synchronized fun resetAutoFill() {
        autoFillType = AUTOFILL_TYPE_NONE
        autoFillImportance = IMPORTANT_FOR_AUTOFILL_NO
        autoFillHints = emptyArray()
    }

    fun getAutoFillManagerService(): AutofillManager? {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) return null
        return try {
            context.getSystemService(AutofillManager::class.java)
        } catch (e: Exception) {
            client?.logStackTraceWithMessage(LOG_TAG, "Failed to get AutofillManager service", e)
            null
        }
    }

    fun isAutoFillEnabled(): Boolean {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) return false
        return try {
            getAutoFillManagerService()?.isEnabled == true
        } catch (e: Exception) {
            client?.logStackTraceWithMessage(LOG_TAG, "Failed to check if Autofill is enabled", e)
            false
        }
    }

    fun requestAutoFillUsername() = requestAutoFill(if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) arrayOf(View.AUTOFILL_HINT_USERNAME) else null)
    fun requestAutoFillPassword() = requestAutoFill(if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) arrayOf(View.AUTOFILL_HINT_PASSWORD) else null)

    fun requestAutoFill(hints: Array<String>?) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O || hints.isNullOrEmpty()) return
        try {
            val afm = getAutoFillManagerService()
            if (afm?.isEnabled == true) {
                autoFillType = AUTOFILL_TYPE_TEXT
                autoFillImportance = IMPORTANT_FOR_AUTOFILL_YES
                autoFillHints = hints
                afm.requestAutofill(this)
            }
        } catch (e: Exception) {
            client?.logStackTraceWithMessage(LOG_TAG, "Failed to request Autofill", e)
        }
    }

    fun cancelRequestAutoFill() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O || autoFillType == AUTOFILL_TYPE_NONE) return
        try {
            getAutoFillManagerService()?.let {
                if (it.isEnabled) {
                    resetAutoFill()
                    it.cancel()
                }
            }
        } catch (e: Exception) {
            client?.logStackTraceWithMessage(LOG_TAG, "Failed to cancel Autofill request", e)
        }
    }

    @Synchronized
    fun setTerminalCursorBlinkerRate(blinkRate: Int): Boolean {
        return if (blinkRate != 0 && (blinkRate < TERMINAL_CURSOR_BLINK_RATE_MIN || blinkRate > TERMINAL_CURSOR_BLINK_RATE_MAX)) {
            client?.logError(LOG_TAG, "The cursor blink rate must be in between $TERMINAL_CURSOR_BLINK_RATE_MIN-$TERMINAL_CURSOR_BLINK_RATE_MAX: $blinkRate")
            terminalCursorBlinkerRate = 0
            false
        } else {
            terminalCursorBlinkerRate = blinkRate
            if (terminalCursorBlinkerRate == 0) stopTerminalCursorBlinker()
            true
        }
    }

    @Synchronized
    fun setTerminalCursorBlinkerState(start: Boolean, startOnlyIfCursorEnabled: Boolean) {
        stopTerminalCursorBlinker()
        val emu = emulator ?: return
        emu.setCursorBlinkingEnabled(false)
        if (start) {
            if (terminalCursorBlinkerRate !in TERMINAL_CURSOR_BLINK_RATE_MIN..TERMINAL_CURSOR_BLINK_RATE_MAX) return
            if (startOnlyIfCursorEnabled && !emu.isCursorEnabled) return

            if (terminalCursorBlinkerHandler == null) terminalCursorBlinkerHandler = Handler(Looper.getMainLooper())
            terminalCursorBlinkerRunnable = TerminalCursorBlinkerRunnable(emu, terminalCursorBlinkerRate)
            emu.setCursorBlinkingEnabled(true)
            terminalCursorBlinkerRunnable?.run()
        }
    }

    private fun stopTerminalCursorBlinker() {
        terminalCursorBlinkerHandler?.removeCallbacks(terminalCursorBlinkerRunnable ?: return)
    }

    private inner class TerminalCursorBlinkerRunnable(private var emu: TerminalEmulator?, private val blinkRate: Int) : Runnable {
        private var cursorVisible = false
        fun setEmulator(emulator: TerminalEmulator?) { emu = emulator }
        override fun run() {
            try {
                emu?.let {
                    cursorVisible = !cursorVisible
                    it.setCursorBlinkState(cursorVisible)
                    invalidate()
                }
            } finally {
                terminalCursorBlinkerHandler?.postDelayed(this, blinkRate.toLong())
            }
        }
    }

    private val textSelectionActionMode: ActionMode?
        get() = textSelectionCursorController?.actionMode

    val isSelectingText: Boolean
        get() = textSelectionCursorController?.isActive == true

    fun startTextSelectionMode(event: MotionEvent) {
        if (!requestFocus()) return
        textSelectionCursorController = textSelectionCursorController ?: TextSelectionCursorController(this).also {
            viewTreeObserver?.addOnTouchModeChangeListener(it)
        }
        textSelectionCursorController?.show(event)
        client?.copyModeChanged(isSelectingText)
        invalidate()
    }

    fun stopTextSelectionMode() {
        if (textSelectionCursorController?.hide() == true) {
            client?.copyModeChanged(isSelectingText)
            invalidate()
        }
    }

    private fun decrementYTextSelectionCursors(decrement: Int) {
        textSelectionCursorController?.decrementYTextSelectionCursors(decrement)
    }

    override fun onAttachedToWindow() {
        super.onAttachedToWindow()
        textSelectionCursorController?.let { viewTreeObserver?.addOnTouchModeChangeListener(it) }
    }

    override fun onDetachedFromWindow() {
        super.onDetachedFromWindow()
        textSelectionCursorController?.let {
            stopTextSelectionMode()
            viewTreeObserver?.removeOnTouchModeChangeListener(it)
            it.onDetached()
        }
    }

    private fun updateFloatingToolbarVisibility(event: MotionEvent) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && textSelectionActionMode != null) {
            when (event.actionMasked) {
                MotionEvent.ACTION_MOVE -> {
                    removeCallbacks(showFloatingToolbarRunnable)
                    textSelectionActionMode?.hide(-1)
                }
                MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                    postDelayed(showFloatingToolbarRunnable, ViewConfiguration.getDoubleTapTimeout().toLong())
                }
            }
        }
    }

    companion object {
        private var terminalViewKeyLoggingEnabled = false
        private const val LOG_TAG = "TerminalView"
        const val TERMINAL_CURSOR_BLINK_RATE_MIN = 100
        const val TERMINAL_CURSOR_BLINK_RATE_MAX = 2000
        const val KEY_EVENT_SOURCE_VIRTUAL_KEYBOARD = KeyCharacterMap.VIRTUAL_KEYBOARD
        const val KEY_EVENT_SOURCE_SOFT_KEYBOARD = 0

        fun setIsTerminalViewKeyLoggingEnabled(value: Boolean) {
            terminalViewKeyLoggingEnabled = value
        }
    }
}
