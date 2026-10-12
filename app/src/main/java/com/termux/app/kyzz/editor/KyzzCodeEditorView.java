package com.termux.app.kyzz.editor;

import android.content.Context;
import android.graphics.Color;
import android.graphics.Typeface;
import android.text.Editable;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.TextWatcher;
import android.text.style.ForegroundColorSpan;
import android.util.AttributeSet;
import android.view.Gravity;

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
    private boolean mIsSyntaxHighlighting = false;

    // Syntax highlighting patterns
    private static final Pattern PATTERN_KEYWORDS = Pattern.compile(
        "\\b(abstract|assert|boolean|break|byte|case|catch|char|class|const|continue|default|do|double|else|enum|extends|final|finally|float|for|goto|if|implements|import|instanceof|int|interface|long|native|new|package|private|protected|public|return|short|static|strictfp|super|switch|synchronized|this|throw|throws|transient|try|void|volatile|while|def|fn|fun|val|var|let|function|async|await|echo|select|from|where|insert|update|delete|create|table|drop|alter)\\b"
    );
    private static final Pattern PATTERN_NUMBERS = Pattern.compile("\\b(\\d+(\\.\\d+)?)\\b");
    private static final Pattern PATTERN_STRINGS = Pattern.compile("\"(\\\\.|[^\"])*\"|'(\\\\.|[^'])*'");
    private static final Pattern PATTERN_COMMENTS = Pattern.compile("(//.*$|#.*$|/\\*.*?\\*/)", Pattern.MULTILINE);

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
        setTextSize(14f);
        setTextColor(Color.parseColor("#e0e6ed"));
        setBackgroundColor(Color.parseColor("#181824"));
        setGravity(Gravity.TOP | Gravity.START);
        setPadding(24, 24, 24, 24);

        addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
                mIsModified = true;
            }

            @Override
            public void afterTextChanged(Editable s) {
                if (mIsSyntaxHighlighting) return;
                applySyntaxHighlighting(s);
            }
        });
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
        setText(content);
        mIsModified = false;
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
    }

    public void detectLanguage(String fileName) {
        String lower = fileName.toLowerCase();
        if (lower.endsWith(".java")) mLanguage = "java";
        else if (lower.endsWith(".kt") || lower.endsWith(".kts")) mLanguage = "kotlin";
        else if (lower.endsWith(".py")) mLanguage = "python";
        else if (lower.endsWith(".js") || lower.endsWith(".jsx")) mLanguage = "javascript";
        else if (lower.endsWith(".ts") || lower.endsWith(".tsx")) mLanguage = "typescript";
        else if (lower.endsWith(".html") || lower.endsWith(".htm")) mLanguage = "html";
        else if (lower.endsWith(".css")) mLanguage = "css";
        else if (lower.endsWith(".json")) mLanguage = "json";
        else if (lower.endsWith(".xml")) mLanguage = "xml";
        else if (lower.endsWith(".sh") || lower.endsWith(".bash")) mLanguage = "shell";
        else if (lower.endsWith(".c") || lower.endsWith(".cpp") || lower.endsWith(".h")) mLanguage = "c/c++";
        else if (lower.endsWith(".php")) mLanguage = "php";
        else if (lower.endsWith(".sql")) mLanguage = "sql";
        else if (lower.endsWith(".md")) mLanguage = "markdown";
        else mLanguage = "text";
    }

    public void applySyntaxHighlighting(Editable editable) {
        if (editable == null || editable.length() > 200000) {
            // Guard against massive files for smooth Android low-end performance
            return;
        }
        mIsSyntaxHighlighting = true;
        try {
            // Remove previous ForegroundColorSpans
            ForegroundColorSpan[] spans = editable.getSpans(0, editable.length(), ForegroundColorSpan.class);
            for (ForegroundColorSpan span : spans) {
                editable.removeSpan(span);
            }

            // Highlight keywords - Purple/Cyan VS Code-style
            Matcher mKeyword = PATTERN_KEYWORDS.matcher(editable);
            while (mKeyword.find()) {
                editable.setSpan(new ForegroundColorSpan(Color.parseColor("#c792ea")),
                        mKeyword.start(), mKeyword.end(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
            }

            // Highlight numbers - Orange
            Matcher mNumber = PATTERN_NUMBERS.matcher(editable);
            while (mNumber.find()) {
                editable.setSpan(new ForegroundColorSpan(Color.parseColor("#f78c6c")),
                        mNumber.start(), mNumber.end(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
            }

            // Highlight strings - Green
            Matcher mString = PATTERN_STRINGS.matcher(editable);
            while (mString.find()) {
                editable.setSpan(new ForegroundColorSpan(Color.parseColor("#c3e88d")),
                        mString.start(), mString.end(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
            }

            // Highlight comments - Grey
            Matcher mComment = PATTERN_COMMENTS.matcher(editable);
            while (mComment.find()) {
                editable.setSpan(new ForegroundColorSpan(Color.parseColor("#676e95")),
                        mComment.start(), mComment.end(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
            }
        } catch (Exception ignored) {
        } finally {
            mIsSyntaxHighlighting = false;
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
