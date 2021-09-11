package com.termux.shared.models;

import android.graphics.Color;
import android.graphics.Typeface;

import androidx.annotation.NonNull;

import com.termux.shared.activities.TextIOActivity;
import com.termux.shared.data.DataUtils;

import java.io.Serializable;

/**
 * An object that stored info for {@link TextIOActivity}.
 * Max text limit is 95KB to prevent TransactionTooLargeException as per
 * {@link DataUtils#TRANSACTION_SIZE_LIMIT_IN_BYTES}. Larger size can be supported for in-app
 * transactions by storing {@link TextIOInfo} as a serialized object in a file like
 * {@link com.termux.shared.activities.ReportActivity} does.
 */
public class TextIOInfo implements Serializable {

    public static final int GENERAL_DATA_SIZE_LIMIT_IN_BYTES = 1000;
    public static final int LABEL_SIZE_LIMIT_IN_BYTES = 4000;
    public static final int TEXT_SIZE_LIMIT_IN_BYTES = 100000 - GENERAL_DATA_SIZE_LIMIT_IN_BYTES - LABEL_SIZE_LIMIT_IN_BYTES; // < 100KB

    /** The action for which {@link TextIOActivity} will be started. */
    private final String mAction;
    /** The internal app component that is will start the {@link TextIOActivity}. */
    private final String mSender;

    /** The activity title. */
    private String mTitle;

    /** If back button should be shown in {@link android.app.ActionBar}. */
    private boolean mShowBackButtonInActionBar = false;


    /** If label is enabled. */
    private boolean mLabelEnabled = false;
    /**
     * The label of text input set in {@link android.widget.TextView} that can be updated by user.
     * Max allowed length is {@link #LABEL_SIZE_LIMIT_IN_BYTES}.
     */
    private String mLabel;
    /** The text size of label. Defaults to 14sp. */
    private int mLabelSize = 14;
    /** The text color of label. Defaults to {@link Color#BLACK}. */
    private int mLabelColor = Color.BLACK;
    /** The {@link Typeface} family  of label. Defaults to "sans-serif". */
    private String mLabelTypeFaceFamily = "sans-serif";
    /** The {@link Typeface} style  of label. Defaults to {@link Typeface#BOLD}. */
    private int mLabelTypeFaceStyle = Typeface.BOLD;


    /**
     * The text of text input set in {@link android.widget.EditText} that can be updated by user.
     * Max allowed length is {@link #TEXT_SIZE_LIMIT_IN_BYTES}.
     */
    private String mText;
    /** The text size for text. Defaults to 12sp. */
    private int mTextSize = 12;
    /** The text size for text. Defaults to {@link #TEXT_SIZE_LIMIT_IN_BYTES}. */
    private int mTextLengthLimit = TEXT_SIZE_LIMIT_IN_BYTES;
    /** The text color of text. Defaults to {@link Color#BLACK}. */
    private int mTextColor = Color.BLACK;
    /** The {@link Typeface} family for text. Defaults to "sans-serif". */
    private String mTextTypeFaceFamily = "sans-serif";
    /** The {@link Typeface} style for text. Defaults to {@link Typeface#NORMAL}. */
    private int mTextTypeFaceStyle = Typeface.NORMAL;
    /** If horizontal scrolling should be enabled for text. */
    private boolean mTextHorizontallyScrolling = false;
    /** If character usage should be enabled for text. */
    private boolean mShowTextCharacterUsage = false;
    /** If editing text should be disabled so that text acts like its in a {@link android.widget.TextView}. */
    private boolean mEditingTextDisabled = false;


    public TextIOInfo(@NonNull String action, @NonNull String sender) {
        mAction = action;
        mSender = sender;
    }


    public String getAction() {
        return mAction;
    }

    public String getSender() {
        return mSender;
    }


    public String getTitle() {
        return mTitle;
    }

    public void setTitle(String title) {
        mTitle = title;
    }

    public boolean shouldShowBackButtonInActionBar() {
        return mShowBackButtonInActionBar;
    }

    public void setShowBackButtonInActionBar(boolean showBackButtonInActionBar) {
        mShowBackButtonInActionBar = showBackButtonInActionBar;
    }


    public boolean isLabelEnabled() {
        return mLabelEnabled;
    }

    public void setLabelEnabled(boolean labelEnabled) {
        mLabelEnabled = labelEnabled;
    }

    public String getLabel() {
        return mLabel;
    }

    public void setLabel(String label) {
        mLabel = DataUtils.getTruncatedCommandOutput(label, LABEL_SIZE_LIMIT_IN_BYTES, true, false, false);
    }

    public int getLabelSize() {
        return mLabelSize;
    }

    public void setLabelSize(int labelSize) {
        if (labelSize > 0)
            mLabelSize = labelSize;
    }

    public int getLabelColor() {
        return mLabelColor;
    }

    public void setLabelColor(int labelColor) {
        mLabelColor = labelColor;
    }

    public String getLabelTypeFaceFamily() {
        return mLabelTypeFaceFamily;
    }

    public void setLabelTypeFaceFamily(String labelTypeFaceFamily) {
        mLabelTypeFaceFamily = labelTypeFaceFamily;
    }

    public int getLabelTypeFaceStyle() {
        return mLabelTypeFaceStyle;
    }

    public void setLabelTypeFaceStyle(int labelTypeFaceStyle) {
        mLabelTypeFaceStyle = labelTypeFaceStyle;
    }


    public String getText() {
        return mText;
    }

    public void setText(String text) {
        mText = DataUtils.getTruncatedCommandOutput(text, TEXT_SIZE_LIMIT_IN_BYTES, true, false, false);
    }

    public int getTextSize() {
        return mTextSize;
    }

    public void setTextSize(int textSize) {
        if (textSize > 0)
            mTextSize = textSize;
    }

    public int getTextLengthLimit() {
        return mTextLengthLimit;
    }

    public void setTextLengthLimit(int textLengthLimit) {
        if (textLengthLimit < TEXT_SIZE_LIMIT_IN_BYTES)
            mTextLengthLimit = textLengthLimit;
    }

    public int getTextColor() {
        return mTextColor;
    }

    public void setTextColor(int textColor) {
        mTextColor = textColor;
    }

    public String getTextTypeFaceFamily() {
        return mTextTypeFaceFamily;
    }

    public void setTextTypeFaceFamily(String textTypeFaceFamily) {
        mTextTypeFaceFamily = textTypeFaceFamily;
    }

    public int getTextTypeFaceStyle() {
        return mTextTypeFaceStyle;
    }

    public void setTextTypeFaceStyle(int textTypeFaceStyle) {
        mTextTypeFaceStyle = textTypeFaceStyle;
    }

    public boolean isHorizontallyScrollable() {
        return mTextHorizontallyScrolling;
    }

    public void setTextHorizontallyScrolling(boolean textHorizontallyScrolling) {
        mTextHorizontallyScrolling = textHorizontallyScrolling;
    }

    public boolean shouldShowTextCharacterUsage() {
        return mShowTextCharacterUsage;
    }

    public void setShowTextCharacterUsage(boolean showTextCharacterUsage) {
        mShowTextCharacterUsage = showTextCharacterUsage;
    }

    public boolean isEditingTextDisabled() {
        return mEditingTextDisabled;
    }

    public void setEditingTextDisabled(boolean editingTextDisabled) {
        mEditingTextDisabled = editingTextDisabled;
    }

}
