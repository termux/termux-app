/*
 *Copyright (C) 1994-2000 The XFree86 Project, Inc. All Rights Reserved.
 *
 *Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 *"Software"), to deal in the Software without restriction, including
 *without limitation the rights to use, copy, modify, merge, publish,
 *distribute, sublicense, and/or sell copies of the Software, and to
 *permit persons to whom the Software is furnished to do so, subject to
 *the following conditions:
 *
 *The above copyright notice and this permission notice shall be
 *included in all copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *NONINFRINGEMENT. IN NO EVENT SHALL THE XFREE86 PROJECT BE LIABLE FOR
 *ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 *CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *Except as contained in this notice, the name of the XFree86 Project
 *shall not be used in advertising or otherwise to promote the sale, use
 *or other dealings in this Software without prior written authorization
 *from the XFree86 Project.
 *
 * Authors:	Dakshinamurthy Karra
 *		Suhaib M Siddiqi
 *		Peter Busch
 *		Harold L Hunt II
 */

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif
#include "win.h"
#include "winkeybd.h"
#include "winconfig.h"
#include "winmsg.h"

#include "xkbsrv.h"

/* C does not have a logical XOR operator, so we use a macro instead */
#define LOGICAL_XOR(a,b) ((!(a) && (b)) || ((a) && !(b)))

static Bool g_winKeyState[NUM_KEYCODES];

/*
 * Local prototypes
 */

static void
 winKeybdBell(int iPercent, DeviceIntPtr pDeviceInt, void *pCtrl, int iClass);

static void
 winKeybdCtrl(DeviceIntPtr pDevice, KeybdCtrl * pCtrl);

/*
 * Translate a Windows WM_[SYS]KEY(UP/DOWN) message
 * into an ASCII scan code.
 *
 * We do this ourselves, rather than letting Windows handle it,
 * because Windows tends to munge the handling of special keys,
 * like AltGr on European keyboards.
 */

int
winTranslateKey(WPARAM wParam, LPARAM lParam)
{
    int iKeyFixup = g_iKeyMap[wParam * WIN_KEYMAP_COLS + 1];
    int iKeyFixupEx = g_iKeyMap[wParam * WIN_KEYMAP_COLS + 2];
    int iParam = HIWORD(lParam);
    int iParamScanCode = LOBYTE(iParam);
    int iScanCode;

    winDebug("winTranslateKey: wParam %08x lParam %08x\n", (int)wParam, (int)lParam);

/* WM_ key messages faked by Vista speech recognition (WSR) don't have a
 * scan code.
 *
 * Vocola 3 (Rick Mohr's supplement to WSR) uses
 * System.Windows.Forms.SendKeys.SendWait(), which appears always to give a
 * scan code of 1
 */
    if (iParamScanCode <= 1) {
        if (VK_PRIOR <= wParam && wParam <= VK_DOWN)
            /* Trigger special case table to translate to extended
             * keycode, otherwise if num_lock is on, we can get keypad
             * numbers instead of navigation keys. */
            iParam |= KF_EXTENDED;
        else
            iParamScanCode = MapVirtualKeyEx(wParam,
                                             /*MAPVK_VK_TO_VSC */ 0,
                                             GetKeyboardLayout(0));
    }

    /* Branch on special extended, special non-extended, or normal key */
    if ((iParam & KF_EXTENDED) && iKeyFixupEx)
        iScanCode = iKeyFixupEx;
    else if (iKeyFixup)
        iScanCode = iKeyFixup;
    else if (wParam == 0 && iParamScanCode == 0x70)
        iScanCode = KEY_HKTG;
    else
        switch (iParamScanCode) {
        case 0x70:
            iScanCode = KEY_HKTG;
            break;
        case 0x73:
            iScanCode = KEY_BSlash2;
            break;
        default:
            iScanCode = iParamScanCode;
            break;
        }

    return iScanCode;
}

/* Ring the keyboard bell (system speaker on PCs) */
static void
winKeybdBell(int iPercent, DeviceIntPtr pDeviceInt, void *pCtrl, int iClass)
{
    /*
     * We can't use Beep () here because it uses the PC speaker
     * on NT/2000.  MessageBeep (MB_OK) will play the default system
     * sound on systems with a sound card or it will beep the PC speaker
     * on systems that do not have a sound card.
     */
    if (iPercent > 0) MessageBeep(MB_OK);
}

/* Change some keyboard configuration parameters */
static void
winKeybdCtrl(DeviceIntPtr pDevice, KeybdCtrl * pCtrl)
{
}

/*
 * See Porting Layer Definition - p. 18
 * winKeybdProc is known as a DeviceProc.
 */

int
winKeybdProc(DeviceIntPtr pDeviceInt, int iState)
{
    DevicePtr pDevice = (DevicePtr) pDeviceInt;
    XkbSrvInfoPtr xkbi;
    XkbControlsPtr ctrl;

    switch (iState) {
    case DEVICE_INIT:
        winConfigKeyboard(pDeviceInt);

        /* FIXME: Maybe we should use winGetKbdLeds () here? */
        defaultKeyboardControl.leds = g_winInfo.keyboard.leds;

        winErrorFVerb(2, "Rules = \"%s\" Model = \"%s\" Layout = \"%s\""
                      " Variant = \"%s\" Options = \"%s\"\n",
                      g_winInfo.xkb.rules ? g_winInfo.xkb.rules : "none",
                      g_winInfo.xkb.model ? g_winInfo.xkb.model : "none",
                      g_winInfo.xkb.layout ? g_winInfo.xkb.layout : "none",
                      g_winInfo.xkb.variant ? g_winInfo.xkb.variant : "none",
                      g_winInfo.xkb.options ? g_winInfo.xkb.options : "none");

        InitKeyboardDeviceStruct(pDeviceInt,
                                 &g_winInfo.xkb, winKeybdBell, winKeybdCtrl);

        xkbi = pDeviceInt->key->xkbInfo;
        if ((xkbi != NULL) && (xkbi->desc != NULL)) {
            ctrl = xkbi->desc->ctrls;
            ctrl->repeat_delay = g_winInfo.keyboard.delay;
            ctrl->repeat_interval = 1000 / g_winInfo.keyboard.rate;
        }
        else {
            winErrorFVerb(1,
                          "winKeybdProc - Error initializing keyboard AutoRepeat\n");
        }

        break;

    case DEVICE_ON:
        pDevice->on = TRUE;

        // immediately copy the state of this keyboard device to the VCK
        // (which otherwise happens lazily after the first keypress)
        CopyKeyClass(pDeviceInt, inputInfo.keyboard);
        break;

    case DEVICE_CLOSE:
    case DEVICE_OFF:
        pDevice->on = FALSE;
        break;
    }

    return Success;
}

/*
 * Detect current mode key states upon server startup.
 *
 * Simulate a press and release of any key that is currently
 * toggled.
 */

void
winInitializeModeKeyStates(void)
{
    /* Restore NumLock */
    if (GetKeyState(VK_NUMLOCK) & 0x0001) {
        winSendKeyEvent(KEY_NumLock, TRUE);
        winSendKeyEvent(KEY_NumLock, FALSE);
    }

    /* Restore CapsLock */
    if (GetKeyState(VK_CAPITAL) & 0x0001) {
        winSendKeyEvent(KEY_CapsLock, TRUE);
        winSendKeyEvent(KEY_CapsLock, FALSE);
    }

    /* Restore ScrollLock */
    if (GetKeyState(VK_SCROLL) & 0x0001) {
        winSendKeyEvent(KEY_ScrollLock, TRUE);
        winSendKeyEvent(KEY_ScrollLock, FALSE);
    }

    /* Restore KanaLock */
    if (GetKeyState(VK_KANA) & 0x0001) {
        winSendKeyEvent(KEY_HKTG, TRUE);
        winSendKeyEvent(KEY_HKTG, FALSE);
    }
}

/*
 * Upon regaining the keyboard focus we must
 * resynchronize our internal mode key states
 * with the actual state of the keys.
 */

void
winRestoreModeKeyStates(void)
{
    DWORD dwKeyState;
    BOOL processEvents = TRUE;
    unsigned short internalKeyStates;

    /* X server is being initialized */
    if (!inputInfo.keyboard || !inputInfo.keyboard->key)
        return;

    /* Only process events if the rootwindow is mapped. The keyboard events
     * will cause segfaults otherwise */
    if (screenInfo.screens[0]->root &&
        screenInfo.screens[0]->root->mapped == FALSE)
        processEvents = FALSE;

    /* Force to process all pending events in the mi event queue */
    if (processEvents)
        mieqProcessInputEvents();

    /* Read the mode key states of our X server */
    /* (stored in the virtual core keyboard) */
    internalKeyStates =
        XkbStateFieldFromRec(&inputInfo.keyboard->key->xkbInfo->state);
    winDebug("winRestoreModeKeyStates: state %d\n", internalKeyStates);

    /* Check if modifier keys are pressed, and if so, fake a press */
    {

        BOOL lctrl = (GetAsyncKeyState(VK_LCONTROL) < 0);
        BOOL rctrl = (GetAsyncKeyState(VK_RCONTROL) < 0);
        BOOL lshift = (GetAsyncKeyState(VK_LSHIFT) < 0);
        BOOL rshift = (GetAsyncKeyState(VK_RSHIFT) < 0);
        BOOL alt = (GetAsyncKeyState(VK_LMENU) < 0);
        BOOL altgr = (GetAsyncKeyState(VK_RMENU) < 0);

        /*
           If AltGr and CtrlL appear to be pressed, assume the
           CtrL is a fake one
         */
        if (lctrl && altgr)
            lctrl = FALSE;

        if (lctrl)
            winSendKeyEvent(KEY_LCtrl, TRUE);

        if (rctrl)
            winSendKeyEvent(KEY_RCtrl, TRUE);

        if (lshift)
            winSendKeyEvent(KEY_ShiftL, TRUE);

        if (rshift)
            winSendKeyEvent(KEY_ShiftL, TRUE);

        if (alt)
            winSendKeyEvent(KEY_Alt, TRUE);

        if (altgr)
            winSendKeyEvent(KEY_AltLang, TRUE);
    }

    /*
       Check if latching modifier key states have changed, and if so,
       fake a press and a release to toggle the modifier to the correct
       state
    */
    dwKeyState = GetKeyState(VK_NUMLOCK) & 0x0001;
    if (LOGICAL_XOR(internalKeyStates & NumLockMask, dwKeyState)) {
        winSendKeyEvent(KEY_NumLock, TRUE);
        winSendKeyEvent(KEY_NumLock, FALSE);
    }

    dwKeyState = GetKeyState(VK_CAPITAL) & 0x0001;
    if (LOGICAL_XOR(internalKeyStates & LockMask, dwKeyState)) {
        winSendKeyEvent(KEY_CapsLock, TRUE);
        winSendKeyEvent(KEY_CapsLock, FALSE);
    }

    dwKeyState = GetKeyState(VK_SCROLL) & 0x0001;
    if (LOGICAL_XOR(internalKeyStates & ScrollLockMask, dwKeyState)) {
        winSendKeyEvent(KEY_ScrollLock, TRUE);
        winSendKeyEvent(KEY_ScrollLock, FALSE);
    }

    dwKeyState = GetKeyState(VK_KANA) & 0x0001;
    if (LOGICAL_XOR(internalKeyStates & KanaMask, dwKeyState)) {
        winSendKeyEvent(KEY_HKTG, TRUE);
        winSendKeyEvent(KEY_HKTG, FALSE);
    }

    /*
       For strict correctness, we should also press any non-modifier keys
       which are already down when we gain focus, but nobody has complained
       yet :-)
     */
}

/*
 * Look for the lovely fake Control_L press/release generated by Windows
 * when AltGr is pressed/released on a non-U.S. keyboard.
 */

Bool
winIsFakeCtrl_L(UINT message, WPARAM wParam, LPARAM lParam)
{
    MSG msgNext;
    LONG lTime;
    Bool fReturn;

    static Bool lastWasControlL = FALSE;
    static LONG lastTime;

    /*
     * Fake Ctrl_L presses will be followed by an Alt_R press
     * with the same timestamp as the Ctrl_L press.
     */
    if ((message == WM_KEYDOWN || message == WM_SYSKEYDOWN)
        && wParam == VK_CONTROL && (HIWORD(lParam) & KF_EXTENDED) == 0) {
        /* Got a Ctrl_L press */

        /* Get time of current message */
        lTime = GetMessageTime();

        /* Look for next press message */
        fReturn = PeekMessage(&msgNext, NULL,
                              WM_KEYDOWN, WM_SYSKEYDOWN, PM_NOREMOVE);

        if (fReturn && msgNext.message != WM_KEYDOWN &&
            msgNext.message != WM_SYSKEYDOWN)
            fReturn = 0;

        if (!fReturn) {
            lastWasControlL = TRUE;
            lastTime = lTime;
        }
        else {
            lastWasControlL = FALSE;
        }

        /* Is next press an Alt_R with the same timestamp? */
        if (fReturn && msgNext.wParam == VK_MENU
            && msgNext.time == lTime
            && (HIWORD(msgNext.lParam) & KF_EXTENDED)) {
            /*
             * Next key press is Alt_R with same timestamp as current
             * Ctrl_L message.  Therefore, this Ctrl_L press is a fake
             * event, so discard it.
             */
            return TRUE;
        }
    }
    /*
     * Sometimes, the Alt_R press message is not yet posted when the
     * fake Ctrl_L press message arrives (even though it has the
     * same timestamp), so check for an Alt_R press message that has
     * arrived since the last Ctrl_L message.
     */
    else if ((message == WM_KEYDOWN || message == WM_SYSKEYDOWN)
             && wParam == VK_MENU && (HIWORD(lParam) & KF_EXTENDED)) {
        /* Got a Alt_R press */

        if (lastWasControlL) {
            lTime = GetMessageTime();

            if (lastTime == lTime) {
                /* Undo the fake Ctrl_L press by sending a fake Ctrl_L release */
                winSendKeyEvent(KEY_LCtrl, FALSE);
            }
            lastWasControlL = FALSE;
        }
    }
    /*
     * Fake Ctrl_L releases will be followed by an Alt_R release
     * with the same timestamp as the Ctrl_L release.
     */
    else if ((message == WM_KEYUP || message == WM_SYSKEYUP)
             && wParam == VK_CONTROL && (HIWORD(lParam) & KF_EXTENDED) == 0) {
        /* Got a Ctrl_L release */

        /* Get time of current message */
        lTime = GetMessageTime();

        /* Look for next release message */
        fReturn = PeekMessage(&msgNext, NULL,
                              WM_KEYUP, WM_SYSKEYUP, PM_NOREMOVE);

        if (fReturn && msgNext.message != WM_KEYUP &&
            msgNext.message != WM_SYSKEYUP)
            fReturn = 0;

        lastWasControlL = FALSE;

        /* Is next press an Alt_R with the same timestamp? */
        if (fReturn
            && (msgNext.message == WM_KEYUP || msgNext.message == WM_SYSKEYUP)
            && msgNext.wParam == VK_MENU
            && msgNext.time == lTime
            && (HIWORD(msgNext.lParam) & KF_EXTENDED)) {
            /*
             * Next key release is Alt_R with same timestamp as current
             * Ctrl_L message. Therefore, this Ctrl_L release is a fake
             * event, so discard it.
             */
            return TRUE;
        }
    }
    else {
        /* On any other press or release message, we don't have a
           potentially fake Ctrl_L to worry about anymore... */
        lastWasControlL = FALSE;
    }

    /* Not a fake control left press/release */
    return FALSE;
}

/*
 * Lift any modifier keys that are pressed
 */

void
winKeybdReleaseKeys(void)
{
    int i;

#ifdef HAS_DEVWINDOWS
    /* Verify that the mi input system has been initialized */
    if (g_fdMessageQueue == WIN_FD_INVALID)
        return;
#endif

    /* Loop through all keys */
    for (i = 0; i < NUM_KEYCODES; ++i) {
        /* Pop key if pressed */
        if (g_winKeyState[i])
            winSendKeyEvent(i, FALSE);

        /* Reset pressed flag for keys */
        g_winKeyState[i] = FALSE;
    }
}

/*
 * Take a raw X key code and send an up or down event for it.
 *
 * Thanks to VNC for inspiration, though it is a simple function.
 */

void
winSendKeyEvent(DWORD dwKey, Bool fDown)
{
    /*
     * When alt-tabing between screens we can get phantom key up messages
     * Here we only pass them through it we think we should!
     */
    if (g_winKeyState[dwKey] == FALSE && fDown == FALSE)
        return;

    /* Update the keyState map */
    g_winKeyState[dwKey] = fDown;

    QueueKeyboardEvents(g_pwinKeyboard, fDown ? KeyPress : KeyRelease,
                        dwKey + MIN_KEYCODE);

    winDebug("winSendKeyEvent: dwKey: %u, fDown: %u\n", (unsigned int)dwKey, fDown);
}

BOOL
winCheckKeyPressed(WPARAM wParam, LPARAM lParam)
{
    switch (wParam) {
    case VK_CONTROL:
        if ((lParam & 0x1ff0000) == 0x11d0000 && g_winKeyState[KEY_RCtrl])
            return TRUE;
        if ((lParam & 0x1ff0000) == 0x01d0000 && g_winKeyState[KEY_LCtrl])
            return TRUE;
        break;
    case VK_SHIFT:
        if ((lParam & 0x1ff0000) == 0x0360000 && g_winKeyState[KEY_ShiftR])
            return TRUE;
        if ((lParam & 0x1ff0000) == 0x02a0000 && g_winKeyState[KEY_ShiftL])
            return TRUE;
        break;
    default:
        return TRUE;
    }
    return FALSE;
}

/* Only one shift release message is sent even if both are pressed.
 * Fix this here
 */
void
winFixShiftKeys(int iScanCode)
{
    if (GetKeyState(VK_SHIFT) & 0x8000)
        return;

    if (iScanCode == KEY_ShiftL && g_winKeyState[KEY_ShiftR])
        winSendKeyEvent(KEY_ShiftR, FALSE);
    if (iScanCode == KEY_ShiftR && g_winKeyState[KEY_ShiftL])
        winSendKeyEvent(KEY_ShiftL, FALSE);
}
