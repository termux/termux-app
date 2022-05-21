package com.termux.view.textselection;

import android.content.ClipboardManager;
import android.content.Context;
import android.graphics.Rect;
import android.view.ActionMode;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;

import com.termux.terminal.TerminalBuffer;
import com.termux.view.R;
import com.termux.view.TerminalView;

public class TextSelectionCursorController implements CursorController {

    private final TerminalView terminalView;
    private final TextSelectionCursorModel textSelectionCursorModel;
    private final TextSelectionHandleView mStartHandle, mEndHandle;

    private ActionMode mActionMode;
    private final int ACTION_COPY = 1;
    private final int ACTION_PASTE = 2;
    private final int ACTION_MORE = 3;

    public TextSelectionCursorController(TerminalView terminalView) {
        this.terminalView = terminalView;

        mStartHandle = new TextSelectionHandleView(terminalView, this, TextSelectionHandleView.LEFT);
        mEndHandle = new TextSelectionHandleView(terminalView, this, TextSelectionHandleView.RIGHT);

        int mHandleHeight = Math.max(mStartHandle.getHandleHeight(), mEndHandle.getHandleHeight());

        this.textSelectionCursorModel = new TextSelectionCursorModel(mHandleHeight);
    }

    @Override
    public void show(MotionEvent event) {
        setInitialTextSelectionPosition(event);

        setHandlerPosition(true);

        setActionModeCallBacks();
        textSelectionCursorModel.setShorStartTime(System.currentTimeMillis());
        textSelectionCursorModel.setIsSelectingText(true);
    }

    @Override
    public boolean hide() {
        if (!isActive()) return false;

        // prevent hide calls right after a show call, like long pressing the down key
        // 300ms seems long enough that it wouldn't cause hide problems if action button
        // is quickly clicked after the show, otherwise decrease it
        if (System.currentTimeMillis() - textSelectionCursorModel.getmShowStartTime() < 300) {
            return false;
        }

        mStartHandle.hide();
        mEndHandle.hide();

        if (mActionMode != null) {
            // This will hide the TextSelectionCursorController
            mActionMode.finish();
        }

        textSelectionCursorModel.setSelectionPosition(-1);
        textSelectionCursorModel.setIsSelectingText(false);

        return true;
    }

    @Override
    public void render() {
        if (!isActive()) return;

        setHandlerPosition(false);

        if (mActionMode != null) {
            mActionMode.invalidate();
        }
    }

    private void setHandlerPosition(boolean forceCheck) {
        mStartHandle.positionAtCursor(textSelectionCursorModel.getmSelX1(), textSelectionCursorModel.getmSelY1(), forceCheck);
        mEndHandle.positionAtCursor(textSelectionCursorModel.getmSelX2() + 1, textSelectionCursorModel.getmSelY2(), forceCheck);
    }

    public void setInitialTextSelectionPosition(MotionEvent event) {
        int[] columnAndRow = terminalView.getColumnAndRow(event, true);
        textSelectionCursorModel.setSelectionPosition(columnAndRow);

        textSelectionCursorModel.setSelectionPositionBlank(terminalView.mEmulator);
    }
    
    public void setActionModeCallBacks() {
        final ActionMode.Callback callback = new ActionMode.Callback() {
            private void addStringOnMenu(Menu menu) {
                int show = MenuItem.SHOW_AS_ACTION_IF_ROOM | MenuItem.SHOW_AS_ACTION_WITH_TEXT;

                ClipboardManager clipboard = (ClipboardManager) terminalView.getContext().getSystemService(Context.CLIPBOARD_SERVICE);
                menu.add(Menu.NONE, ACTION_COPY, Menu.NONE, R.string.copy_text).setShowAsAction(show);
                menu.add(Menu.NONE, ACTION_PASTE, Menu.NONE, R.string.paste_text).setEnabled(clipboard.hasPrimaryClip()).setShowAsAction(show);
                menu.add(Menu.NONE, ACTION_MORE, Menu.NONE, R.string.text_selection_more);
            }

            @Override
            public boolean onCreateActionMode(ActionMode mode, Menu menu) {
                addStringOnMenu(menu);
                return true;
            }

            @Override
            public boolean onPrepareActionMode(ActionMode mode, Menu menu) {
                return false;
            }

            @Override
            public boolean onActionItemClicked(ActionMode mode, MenuItem item) {
                if (!isActive()) {
                    // Fix issue where the dialog is pressed while being dismissed.
                    return true;
                }

                switch (item.getItemId()) {
                    case ACTION_COPY:
                        int[] selPosArr = textSelectionCursorModel.getSelPos();
                        String selectedText = terminalView.mEmulator.getSelectedText(selPosArr[0], selPosArr[1], selPosArr[2], selPosArr[3]).trim();
                        terminalView.mTermSession.onCopyTextToClipboard(selectedText);
                        terminalView.stopTextSelectionMode();
                        break;
                    case ACTION_PASTE:
                        terminalView.stopTextSelectionMode();
                        terminalView.mTermSession.onPasteTextFromClipboard();
                        break;
                    case ACTION_MORE:
                        terminalView.stopTextSelectionMode(); //we stop text selection first, otherwise handles will show above popup
                        terminalView.showContextMenu();
                        break;
                }

                return true;
            }

            @Override
            public void onDestroyActionMode(ActionMode mode) {
            }

        };

        mActionMode = terminalView.startActionMode(new ActionMode.Callback2() {
            @Override
            public boolean onCreateActionMode(ActionMode mode, Menu menu) {
                return callback.onCreateActionMode(mode, menu);
            }

            @Override
            public boolean onPrepareActionMode(ActionMode mode, Menu menu) {
                return false;
            }

            @Override
            public boolean onActionItemClicked(ActionMode mode, MenuItem item) {
                return callback.onActionItemClicked(mode, item);
            }

            @Override
            public void onDestroyActionMode(ActionMode mode) {
                // Ignore.
            }

            @Override
            public void onGetContentRect(ActionMode mode, View view, Rect outRect) {
                int[] selPosArr = textSelectionCursorModel.getSelPos();
                int x1 = Math.round(selPosArr[0] * terminalView.mRenderer.getFontWidth());
                int x2 = Math.round(selPosArr[2] * terminalView.mRenderer.getFontWidth());
                int y1 = Math.round((selPosArr[1] - 1 - terminalView.getTopRow()) * terminalView.mRenderer.getFontLineSpacing());
                int y2 = Math.round((selPosArr[3] + 1 - terminalView.getTopRow()) * terminalView.mRenderer.getFontLineSpacing());

                if (x1 > x2) {
                    int tmp = x1;
                    x1 = x2;
                    x2 = tmp;
                }

                int terminalBottom = terminalView.getBottom();
                int handleHeight = textSelectionCursorModel.getmHandleHeight();
                int top = y1 + handleHeight;
                int bottom = y2 + handleHeight;
                if (top > terminalBottom) top = terminalBottom;
                if (bottom > terminalBottom) bottom = terminalBottom;

                outRect.set(x1, top, x2, bottom);
            }
        }, ActionMode.TYPE_FLOATING);
    }

    @Override
    public void updatePosition(TextSelectionHandleView handle, int x, int y) {
        if (handle == mStartHandle) {
            textSelectionCursorModel.updatePosAtStartHandle(x, y, terminalView);
        } else {
            textSelectionCursorModel.updatePosAtEndHandle(x, y, terminalView);
        }

        terminalView.invalidate();
    }

    public void decrementYTextSelectionCursors(int decrement) {
        textSelectionCursorModel.decrementYSelectionPos(decrement);
    }

    public boolean onTouchEvent(MotionEvent event) {
        return false;
    }

    public void onTouchModeChanged(boolean isInTouchMode) {
        if (!isInTouchMode) {
            terminalView.stopTextSelectionMode();
        }
    }

    @Override
    public void onDetached() {
    }

    @Override
    public boolean isActive() {
        return textSelectionCursorModel.getIsSelcetingText();
    }

    public void getSelectors(int[] sel) {
        if (sel == null || sel.length != 4) {
            return;
        }

        int[] selPosArr = textSelectionCursorModel.getSelPos();
        sel[0] = selPosArr[1];
        sel[1] = selPosArr[3];
        sel[2] = selPosArr[0];
        sel[3] = selPosArr[2];
    }

    public ActionMode getActionMode() {
        return mActionMode;
    }

    /**
     * @return true if this controller is currently used to move the start selection.
     */
    public boolean isSelectionStartDragged() {
        return mStartHandle.isDragging();
    }

    /**
     * @return true if this controller is currently used to move the end selection.
     */
    public boolean isSelectionEndDragged() {
        return mEndHandle.isDragging();
    }

}
