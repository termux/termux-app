package com.termux.app.kyzz.editor;

import android.content.Context;
import android.graphics.Color;
import android.graphics.Typeface;
import android.text.Editable;
import android.text.InputType;
import android.text.Spannable;
import android.text.TextWatcher;
import android.text.style.ForegroundColorSpan;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.inputmethod.EditorInfo;

import androidx.appcompat.widget.AppCompatEditText;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class KyzzCodeEditorView extends AppCompatEditText {
    private File mCurrentFile;
    private boolean mIsModified = false;
    private String mLanguage = "text";
    private boolean mIsFormatting = false;

    // Syntax highlighting patterns
    private static final Pattern PATTERN_KEYWORDS = Pattern.compile(
        "\\b(abstract|assert|boolean|break|byte|case|catch|char|class|const|continue|default|do|double|else|enum|extends|final|finally|float|for|goto|if|implements|import|instanceof|int|interface|long|native|new|package|private|protected|public|return|short|static|strictfp|super|switch|synchronized|this|throw|throws|transient|try|void|volatile|while|def|fn|fun|val|var|let|function|async|await|echo|select|from|where|insert|update|delete|create|table|drop|alter)\\b"
    );
    private static final Pattern PATTERN_NUMBERS = Pattern.compile("\\b(\\d+(\\.\\d+)?)\\b");
    private static final Pattern PATTERN_STRINGS = Pattern.compile("\"(\\\\.|[^\"])*\"|'(\\\\.|[^'])*'");
    private static final Pattern PATTERN_COMMENTS = Pattern.compile("(//.*$|#.*$|/\\*.*?\\*/)", Pattern.MULTILINE);

    public interface OnCursorChangeListener {
        void onCursorChanged(int line, int col, String language);
    }

    private OnCursorChangeListener mCursorListener;

    public KyzzCodeEditorView(Context context) {
        super(context);
        init();
    }

    public KyzzCodeEditorView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    private void init() {
        setTypeface(Typeface.MONOSPACE);
        setTextSize(13f);
        setTextColor(Color.parseColor("#e0e6ed"));
        setBackgroundColor(Color.TRANSPARENT);
        setGravity(Gravity.TOP | Gravity.START);
        setPadding(20, 20, 20, 20);

        // Enable multi-line code editing without soft keyboard auto-correct mangling code
        setInputType(InputType.TYPE_CLASS_TEXT |
                     InputType.TYPE_TEXT_FLAG_MULTI_LINE |
                     InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS);
        setImeOptions(EditorInfo.IME_FLAG_NO_FULLSCREEN);
        setHorizontallyScrolling(true);

        addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
                if (!mIsFormatting) {
                    mIsModified = true;
                }
            }

            @Override
            public void afterTextChanged(Editable s) {
                if (mIsFormatting) return;
                applySyntaxHighlighting(s);
                notifyCursorChanged();
            }
        });
    }

    public void setOnCursorChangeListener(OnCursorChangeListener listener) {
        mCursorListener = listener;
    }

    @Override
    protected void onSelectionChanged(int selStart, int selEnd) {
        super.onSelectionChanged(selStart, selEnd);
        notifyCursorChanged();
    }

    private void notifyCursorChanged() {
        if (mCursorListener != null) {
            mCursorListener.onCursorChanged(getCurrentLineNumber(), getCurrentColumnNumber(), mLanguage);
        }
    }

    public void openFile(File file) throws IOException {
        if (!file.exists()) {
            throw new IOException("File does not exist: " + file.getAbsolutePath());
        }
        mCurrentFile = file;
        detectLanguage(file.getName());

        FileInputStream fis = new FileInputStream(file);
        byte[] data = new byte[(int) file.length()];
        int read = fis.read(data);
        fis.close();

        String content = new String(data, 0, Math.max(0, read), StandardCharsets.UTF_8);
        mIsFormatting = true;
        setText(content);
        mIsFormatting = false;
        mIsModified = false;
        applySyntaxHighlighting(getText());
        notifyCursorChanged();
    }

    public void saveFile() throws IOException {
        if (mCurrentFile == null) {
            throw new IOException("No target file specified");
        }
        saveAs(mCurrentFile);
    }

    public void saveAs(File target) throws IOException {
        if (target == null) throw new IOException("Target file is null");
        String content = getText() != null ? getText().toString() : "";
        FileOutputStream fos = new FileOutputStream(target);
        fos.write(content.getBytes(StandardCharsets.UTF_8));
        fos.flush();
        fos.close();
        mCurrentFile = target;
        detectLanguage(target.getName());
        mIsModified = false;
        notifyCursorChanged();
    }

    public void detectLanguage(String fileName) {
        String lower = fileName.toLowerCase();
        if (lower.endsWith(".java")) mLanguage = "Java";
        else if (lower.endsWith(".kt") || lower.endsWith(".kts")) mLanguage = "Kotlin";
        else if (lower.endsWith(".py")) mLanguage = "Python";
        else if (lower.endsWith(".js") || lower.endsWith(".jsx")) mLanguage = "JavaScript";
        else if (lower.endsWith(".ts") || lower.endsWith(".tsx")) mLanguage = "TypeScript";
        else if (lower.endsWith(".html") || lower.endsWith(".htm")) mLanguage = "HTML";
        else if (lower.endsWith(".css")) mLanguage = "CSS";
        else if (lower.endsWith(".json")) mLanguage = "JSON";
        else if (lower.endsWith(".xml")) mLanguage = "XML";
        else if (lower.endsWith(".sh") || lower.endsWith(".bash")) mLanguage = "Shell";
        else if (lower.endsWith(".c") || lower.endsWith(".cpp") || lower.endsWith(".h")) mLanguage = "C/C++";
        else if (lower.endsWith(".php")) mLanguage = "PHP";
        else if (lower.endsWith(".sql")) mLanguage = "SQL";
        else if (lower.endsWith(".md")) mLanguage = "Markdown";
        else mLanguage = "Plain Text";
    }

    public void applySyntaxHighlighting(Editable editable) {
        if (editable == null || editable.length() > 100000) {
            return;
        }
        mIsFormatting = true;
        try {
            ForegroundColorSpan[] spans = editable.getSpans(0, editable.length(), ForegroundColorSpan.class);
            for (ForegroundColorSpan span : spans) {
                editable.removeSpan(span);
            }

            Matcher mKeyword = PATTERN_KEYWORDS.matcher(editable);
            while (mKeyword.find()) {
                editable.setSpan(new ForegroundColorSpan(Color.parseColor("#c792ea")),
                        mKeyword.start(), mKeyword.end(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
            }

            Matcher mNumber = PATTERN_NUMBERS.matcher(editable);
            while (mNumber.find()) {
                editable.setSpan(new ForegroundColorSpan(Color.parseColor("#f78c6c")),
                        mNumber.start(), mNumber.end(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
            }

            Matcher mString = PATTERN_STRINGS.matcher(editable);
            while (mString.find()) {
                editable.setSpan(new ForegroundColorSpan(Color.parseColor("#c3e88d")),
                        mString.start(), mString.end(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
            }

            Matcher mComment = PATTERN_COMMENTS.matcher(editable);
            while (mComment.find()) {
                editable.setSpan(new ForegroundColorSpan(Color.parseColor("#676e95")),
                        mComment.start(), mComment.end(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
            }
        } catch (Exception ignored) {
        } finally {
            mIsFormatting = false;
        }
    }

    public File getCurrentFile() {
        return mCurrentFile;
    }

    public boolean isModified() {
        return mIsModified;
    }

    public String getLanguage() {
        return mLanguage;
    }

    public int getCurrentLineNumber() {
        int pos = getSelectionStart();
        if (pos < 0 || getText() == null) return 1;
        int line = 1;
        String text = getText().toString();
        for (int i = 0; i < pos && i < text.length(); i++) {
            if (text.charAt(i) == '\n') line++;
        }
        return line;
    }

    public int getCurrentColumnNumber() {
        int pos = getSelectionStart();
        if (pos < 0 || getText() == null) return 1;
        int col = 1;
        String text = getText().toString();
        for (int i = pos - 1; i >= 0 && i < text.length(); i--) {
            if (text.charAt(i) == '\n') break;
            col++;
        }
        return col;
    }

    public void goToLine(int line) {
        if (getText() == null || line <= 1) {
            setSelection(0);
            return;
        }
        String text = getText().toString();
        int curLine = 1;
        for (int i = 0; i < text.length(); i++) {
            if (curLine == line) {
                setSelection(i);
                return;
            }
            if (text.charAt(i) == '\n') curLine++;
        }
        setSelection(text.length());
    }
}
