package com.termux.shared.activities;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.graphics.Typeface;
import android.os.Bundle;
import android.text.Editable;
import android.text.InputFilter;
import android.text.TextWatcher;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.HorizontalScrollView;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;

import com.termux.shared.interact.ShareUtils;
import com.termux.shared.logger.Logger;
import com.termux.shared.R;
import com.termux.shared.models.TextIOInfo;
import com.termux.shared.view.KeyboardUtils;

import org.jetbrains.annotations.NotNull;

import java.util.Locale;

/**
 * An activity to edit or view text based on config passed as {@link TextIOInfo}.
 *
 * Add Following to `AndroidManifest.xml` to use in an app:
 *
 * {@code ` <activity android:name="com.termux.shared.activities.TextIOActivity" android:theme="@style/Theme.AppCompat.TermuxTextIOActivity" />` }
 */
public class TextIOActivity extends AppCompatActivity {

    private static final String CLASS_NAME = ReportActivity.class.getCanonicalName();
    public static final String EXTRA_TEXT_IO_INFO_OBJECT = CLASS_NAME + ".EXTRA_TEXT_IO_INFO_OBJECT";

    private TextView mTextIOLabel;
    private View mTextIOLabelSeparator;
    private EditText mTextIOText;
    private HorizontalScrollView mTextIOHorizontalScrollView;
    private LinearLayout mTextIOTextLinearLayout;
    private TextView mTextIOTextCharacterUsage;

    private TextIOInfo mTextIOInfo;
    private Bundle mBundle;

    private static final String LOG_TAG = "TextIOActivity";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Logger.logVerbose(LOG_TAG, "onCreate");

        setContentView(R.layout.activity_text_io);

        mTextIOLabel = findViewById(R.id.text_io_label);
        mTextIOLabelSeparator = findViewById(R.id.text_io_label_separator);
        mTextIOText = findViewById(R.id.text_io_text);
        mTextIOHorizontalScrollView = findViewById(R.id.text_io_horizontal_scroll_view);
        mTextIOTextLinearLayout = findViewById(R.id.text_io_text_linear_layout);
        mTextIOTextCharacterUsage = findViewById(R.id.text_io_text_character_usage);

        Toolbar toolbar = findViewById(R.id.toolbar);
        if (toolbar != null) {
            setSupportActionBar(toolbar);
        }

        mBundle = null;
        Intent intent = getIntent();
        if (intent != null)
            mBundle = intent.getExtras();
        else if (savedInstanceState != null)
            mBundle = savedInstanceState;

        updateUI();
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        Logger.logVerbose(LOG_TAG, "onNewIntent");

        // Views must be re-created since different configs for isEditingTextDisabled() and
        // isHorizontallyScrollable() will not work or at least reliably
        finish();
        startActivity(intent);
    }

    @SuppressLint("ClickableViewAccessibility")
    private void updateUI() {
        if (mBundle == null) {
            finish(); return;
        }

        mTextIOInfo = (TextIOInfo) mBundle.getSerializable(EXTRA_TEXT_IO_INFO_OBJECT);
        if (mTextIOInfo == null) {
            finish(); return;
        }

        final ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            if (mTextIOInfo.getTitle() != null)
                actionBar.setTitle(mTextIOInfo.getTitle());
            else
                actionBar.setTitle("Text Input");

            if (mTextIOInfo.shouldShowBackButtonInActionBar()) {
                actionBar.setDisplayHomeAsUpEnabled(true);
                actionBar.setDisplayShowHomeEnabled(true);
            }
        }

        mTextIOLabel.setVisibility(View.GONE);
        mTextIOLabelSeparator.setVisibility(View.GONE);
        if (mTextIOInfo.isLabelEnabled()) {
            mTextIOLabel.setVisibility(View.VISIBLE);
            mTextIOLabelSeparator.setVisibility(View.VISIBLE);
            mTextIOLabel.setText(mTextIOInfo.getLabel());
            mTextIOLabel.setFilters(new InputFilter[] { new InputFilter.LengthFilter(TextIOInfo.LABEL_SIZE_LIMIT_IN_BYTES) });
            mTextIOLabel.setTextSize(mTextIOInfo.getLabelSize());
            mTextIOLabel.setTextColor(mTextIOInfo.getLabelColor());
            mTextIOLabel.setTypeface(Typeface.create(mTextIOInfo.getLabelTypeFaceFamily(), mTextIOInfo.getLabelTypeFaceStyle()));
        }


        if (mTextIOInfo.isHorizontallyScrollable()) {
            mTextIOHorizontalScrollView.setEnabled(true);
            mTextIOText.setHorizontallyScrolling(true);
        } else {
            // Remove mTextIOHorizontalScrollView and add mTextIOText in its place
            ViewGroup parent = (ViewGroup) mTextIOHorizontalScrollView.getParent();
            if (parent != null && parent.indexOfChild(mTextIOText) < 0) {
                ViewGroup.LayoutParams params = mTextIOHorizontalScrollView.getLayoutParams();
                int index = parent.indexOfChild(mTextIOHorizontalScrollView);
                mTextIOTextLinearLayout.removeAllViews();
                mTextIOHorizontalScrollView.removeAllViews();
                parent.removeView(mTextIOHorizontalScrollView);
                parent.addView(mTextIOText, index, params);
                mTextIOText.setHorizontallyScrolling(false);
            }
        }

        mTextIOText.setText(mTextIOInfo.getText());
        mTextIOText.setFilters(new InputFilter[] { new InputFilter.LengthFilter(mTextIOInfo.getTextLengthLimit()) });
        mTextIOText.setTextSize(mTextIOInfo.getTextSize());
        mTextIOText.setTextColor(mTextIOInfo.getTextColor());
        mTextIOText.setTypeface(Typeface.create(mTextIOInfo.getTextTypeFaceFamily(), mTextIOInfo.getTextTypeFaceStyle()));

        // setTextIsSelectable must be called after changing KeyListener to regain focusability and selectivity
        if (mTextIOInfo.isEditingTextDisabled()) {
            mTextIOText.setCursorVisible(false);
            mTextIOText.setKeyListener(null);
            mTextIOText.setTextIsSelectable(true);
        }

        if (mTextIOInfo.shouldShowTextCharacterUsage()) {
            mTextIOTextCharacterUsage.setVisibility(View.VISIBLE);
            updateTextIOTextCharacterUsage(mTextIOInfo.getText());

            mTextIOText.addTextChangedListener(new TextWatcher() {
                @Override
                public void beforeTextChanged(CharSequence s, int start, int count, int after) {}
                @Override
                public void onTextChanged(CharSequence s, int start, int before, int count) {}
                @Override
                public void afterTextChanged(Editable editable) {
                    if (editable != null)
                        updateTextIOTextCharacterUsage(editable.toString());
                }
            });
        } else {
            mTextIOTextCharacterUsage.setVisibility(View.GONE);
            mTextIOText.addTextChangedListener(null);
        }
    }

    private void updateTextIOInfoText() {
        if (mTextIOText != null)
            mTextIOInfo.setText(mTextIOText.getText().toString());
    }

    private void updateTextIOTextCharacterUsage(String text) {
        if (text == null) text = "";
        if (mTextIOTextCharacterUsage != null)
            mTextIOTextCharacterUsage.setText(String.format(Locale.getDefault(), "%1$d/%2$d", text.length(), mTextIOInfo.getTextLengthLimit()));
    }

    @Override
    public void onSaveInstanceState(@NonNull Bundle outState) {
        super.onSaveInstanceState(outState);

        updateTextIOInfoText();
        outState.putSerializable(EXTRA_TEXT_IO_INFO_OBJECT, mTextIOInfo);
    }

    @Override
    public boolean onCreateOptionsMenu(final Menu menu) {
        final MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.menu_text_io, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(final MenuItem item) {
        String text = "";
        if (mTextIOText != null)
            text = mTextIOText.getText().toString();

        int id = item.getItemId();
        if (id == android.R.id.home) {
            confirm();
        } if (id == R.id.menu_item_cancel) {
            cancel();
        } else if (id == R.id.menu_item_share_text) {
            ShareUtils.shareText(this, mTextIOInfo.getTitle(), text);
        } else if (id == R.id.menu_item_copy_text) {
            ShareUtils.copyTextToClipboard(this, text, null);
        }

        return false;
    }

    @Override
    public void onBackPressed() {
        confirm();
    }

    /** Confirm current text and send it back to calling {@link Activity}. */
    private void confirm() {
        updateTextIOInfoText();
        KeyboardUtils.hideSoftKeyboard(this, mTextIOText);
        setResult(Activity.RESULT_OK, getResultIntent());
        finish();
    }

    /** Cancel current text and notify calling {@link Activity}. */
    private void cancel() {
        KeyboardUtils.hideSoftKeyboard(this, mTextIOText);
        setResult(Activity.RESULT_CANCELED, getResultIntent());
        finish();
    }

    @NotNull
    private Intent getResultIntent() {
        Intent intent = new Intent();
        Bundle bundle = new Bundle();
        bundle.putSerializable(EXTRA_TEXT_IO_INFO_OBJECT, mTextIOInfo);
        intent.putExtras(bundle);
        return intent;
    }

    /**
     * Get the {@link Intent} that can be used to start the {@link TextIOActivity}.
     *
     * @param context The {@link Context} for operations.
     * @param textIOInfo The {@link TextIOInfo} containing info for the edit text.
     */
    public static Intent newInstance(@NonNull final Context context, @NonNull final TextIOInfo textIOInfo) {
        Intent intent = new Intent(context, TextIOActivity.class);
        Bundle bundle = new Bundle();
        bundle.putSerializable(EXTRA_TEXT_IO_INFO_OBJECT, textIOInfo);
        intent.putExtras(bundle);
        return intent;
    }

}
