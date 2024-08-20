package com.termux.x11.extrakeys;

import android.widget.Button;

import java.util.ArrayList;
import java.util.List;

/** The {@link Class} that maintains a state of a {@link TermuxX11SpecialButton} */
public class TermuxX11SpecialButtonState {

    /** If special button has been created for the {@link TermuxExtraKeysView}. */
    boolean isCreated = false;
    /** If special button is active. */
    boolean isActive = false;
    /** If special button is locked due to long hold on it and should not be deactivated if its
     * state is read. */
    boolean isLocked = false;

    List<Button> buttons = new ArrayList<>();

    TermuxExtraKeysView mTermuxExtraKeysView;

    /**
     * Initialize a {@link TermuxX11SpecialButtonState} to maintain state of a {@link TermuxX11SpecialButton}.
     *
     * @param termuxExtraKeysView The {@link TermuxExtraKeysView} instance in which the {@link TermuxX11SpecialButton}
     *                      is to be registered.
     */
    public TermuxX11SpecialButtonState(TermuxExtraKeysView termuxExtraKeysView) {
        mTermuxExtraKeysView = termuxExtraKeysView;
    }

    /** Set {@link #isCreated}. */
    public void setIsCreated(boolean value) {
        isCreated = value;
    }

    /** Set {@link #isActive}. */
    public void setIsActive(boolean value) {
        isActive = value;
        for (Button button : buttons) {
            button.setTextColor(value ? mTermuxExtraKeysView.getButtonActiveTextColor() : mTermuxExtraKeysView.getButtonTextColor());
        }
    }

    /** Set {@link #isLocked}. */
    public void setIsLocked(boolean value) {
        isLocked = value;
    }
}
