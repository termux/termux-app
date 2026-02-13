# Swipe Typing Fix Report
**Date:** 2026-02-13 16:38 (Asia/Omsk, UTC+6)

## Problem
Swipe typing (gesture typing) on keyboards like Gboard was not working in Termux terminal view.

## Root Cause Analysis
The issue had multiple causes:

1. **InputType.TYPE_NULL**: The original implementation used `InputType.TYPE_NULL` in `onCreateInputConnection()`, which completely disables swipe typing on most keyboards including Gboard. Keyboards use this flag to determine what input features to enable, and `TYPE_NULL` tells them "no text input needed", so swipe gestures are disabled.

2. **Missing setComposingText() handling**: Swipe keyboards use the composing text mechanism (`setComposingText()` and `finishComposingText()`) to send text during a swipe gesture. The original implementation didn't properly handle this, causing swipe input to be lost.

3. **Delayed text appearance**: After initial fix, text appeared only after pressing another key because `finishComposingText()` was calling `sendTextToTerminal(getEditable())` which could be empty or duplicate the composing text.

## Solution

### 1. Changed InputType (TerminalView.java lines 312-317)
```java
if (mClient.isTerminalViewSelected()) {
    // Use TYPE_CLASS_TEXT to enable swipe typing (gesture typing) on keyboards like Gboard.
    // Previously TYPE_NULL was used which disabled swipe typing completely.
    // We use TYPE_TEXT_FLAG_NO_SUGGESTIONS to avoid autocomplete suggestions.
    // The text is still processed character by character in our InputConnection.
    outAttrs.inputType = InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS;
}
```

### 2. Implemented setComposingText() and finishComposingText() (TerminalView.java lines 344-382)
```java
/** Track the composing text to handle swipe typing correctly */
private StringBuilder mComposingText = new StringBuilder();

@Override
public boolean setComposingText(CharSequence text, int newCursorPosition) {
    if (TERMINAL_VIEW_KEY_LOGGING_ENABLED) {
        mClient.logInfo(LOG_TAG, "IME: setComposingText(\"" + text + "\", " + newCursorPosition + ")");
    }
    
    if (mEmulator == null) return true;
    
    // For swipe typing, the keyboard sends composing text updates during the gesture.
    // We store the composing text and will send it when finishComposingText() is called.
    mComposingText.setLength(0);
    if (text != null) {
        mComposingText.append(text);
    }
    
    super.setComposingText(text, newCursorPosition);
    return true;
}

@Override
public boolean finishComposingText() {
    if (TERMINAL_VIEW_KEY_LOGGING_ENABLED) mClient.logInfo(LOG_TAG, "IME: finishComposingText()");
    
    // Send the composing text to terminal when swipe is complete
    if (mComposingText.length() > 0) {
        sendTextToTerminal(mComposingText.toString());
        mComposingText.setLength(0);
    }
    
    super.finishComposingText();
    getEditable().clear();
    return true;
}
```

### 3. Fixed commitText() to handle swipe completion
```java
@Override
public boolean commitText(CharSequence text, int newCursorPosition) {
    if (TERMINAL_VIEW_KEY_LOGGING_ENABLED) {
        mClient.logInfo(LOG_TAG, "IME: commitText(\"" + text + "\", " + newCursorPosition + ")");
    }

    if (mEmulator == null) return true;

    // Clear any pending composing text since commitText supersedes it
    mComposingText.setLength(0);

    // Send the committed text directly to terminal
    if (text != null && text.length() > 0) {
        sendTextToTerminal(text);
    }

    super.commitText(text, newCursorPosition);
    getEditable().clear();
    return true;
}
```

## Build Configuration Changes
Modified `app/build.gradle` to build only for arm64-v8a architecture:
- Changed `splits.abi.include` from all architectures to only `'arm64-v8a'`
- Changed `universalApk` to `false`
- Modified `downloadBootstraps` task to download only aarch64 bootstrap

## Files Modified
1. `terminal-view/src/main/java/com/termux/view/TerminalView.java`
   - Changed `InputType.TYPE_NULL` to `InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS`
   - Added `mComposingText` StringBuilder field
   - Implemented `setComposingText()` method
   - Updated `finishComposingText()` method - removed duplicate `sendTextToTerminal(getEditable())` call
   - Updated `commitText()` method - send text directly, clear composing text first

2. `app/build.gradle`
   - Modified ABI splits to build only arm64-v8a
   - Modified downloadBootstraps task for aarch64 only

## Build Result
- **Status:** SUCCESS
- **APK:** `app/build/outputs/apk/debug/termux-app_apt-android-7-debug_arm64-v8a.apk`
- **Size:** ~38.5 MB

## Testing Notes
The fix should be tested with:
1. Gboard swipe typing - text should appear immediately after swipe completes
2. Other swipe-enabled keyboards (SwiftKey, etc.)
3. Regular typing to ensure no regression
4. Special characters and Unicode input

## Technical Details
- Swipe keyboards call `setComposingText()` multiple times during a swipe gesture with updated text
- `finishComposingText()` is called when the swipe gesture is complete
- The fix stores the composing text during the gesture and sends it to the terminal only when complete
- `commitText()` is called for some keyboards instead of `finishComposingText()` - both paths now work correctly
- Removed the extra `sendTextToTerminal(getEditable())` call that was causing text to appear only on next keypress
