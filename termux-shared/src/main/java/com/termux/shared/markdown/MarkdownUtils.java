package com.termux.shared.markdown;

import android.content.Context;
import android.graphics.Typeface;
import android.text.Spanned;
import android.text.style.AbsoluteSizeSpan;
import android.text.style.BackgroundColorSpan;
import android.text.style.BulletSpan;
import android.text.style.QuoteSpan;
import android.text.style.StrikethroughSpan;
import android.text.style.StyleSpan;
import android.text.style.TypefaceSpan;
import android.text.util.Linkify;

import androidx.annotation.NonNull;
import androidx.core.content.ContextCompat;

import com.google.common.base.Strings;
import com.termux.shared.R;
import com.termux.shared.theme.ThemeUtils;

import org.commonmark.ext.gfm.strikethrough.Strikethrough;
import org.commonmark.node.BlockQuote;
import org.commonmark.node.Code;
import org.commonmark.node.Emphasis;
import org.commonmark.node.FencedCodeBlock;
import org.commonmark.node.ListItem;
import org.commonmark.node.StrongEmphasis;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

import io.noties.markwon.AbstractMarkwonPlugin;
import io.noties.markwon.Markwon;
import io.noties.markwon.MarkwonSpansFactory;
import io.noties.markwon.MarkwonVisitor;
import io.noties.markwon.ext.strikethrough.StrikethroughPlugin;
import io.noties.markwon.linkify.LinkifyPlugin;

public class MarkdownUtils {

    public static final String backtick = "`";
    public static final Pattern backticksPattern = Pattern.compile("(" + backtick + "+)");

    /**
     * Get the markdown code {@link String} for a {@link String}. This ensures all backticks "`" are
     * properly escaped so that markdown does not break.
     *
     * @param string The {@link String} to convert.
     * @param codeBlock If the {@link String} is to be converted to a code block or inline code.
     * @return Returns the markdown code {@link String}.
     */
    public static String getMarkdownCodeForString(String string, boolean codeBlock) {
        if (string == null) return null;
        if (string.isEmpty()) return "";

        int maxConsecutiveBackTicksCount = getMaxConsecutiveBackTicksCount(string);

        // markdown requires surrounding backticks count to be at least one more than the count
        // of consecutive ticks in the string itself
        int backticksCountToUse;
        if (codeBlock)
            backticksCountToUse = maxConsecutiveBackTicksCount + 3;
        else
            backticksCountToUse = maxConsecutiveBackTicksCount + 1;

        // create a string with n backticks where n==backticksCountToUse
        String backticksToUse = Strings.repeat(backtick, backticksCountToUse);

        if (codeBlock)
            return backticksToUse + "\n" + string + "\n" + backticksToUse;
        else {
            // add a space to any prefixed or suffixed backtick characters
            if (string.startsWith(backtick))
                string = " " + string;
            if (string.endsWith(backtick))
                string = string + " ";

            return backticksToUse + string + backticksToUse;
        }
    }

    /**
     * Get the max consecutive backticks "`" in a {@link String}.
     *
     * @param string The {@link String} to check.
     * @return Returns the max consecutive backticks count.
     */
    public static int getMaxConsecutiveBackTicksCount(String string) {
        if (string == null || string.isEmpty()) return 0;

        int maxCount = 0;
        int matchCount;
        String match;

        Matcher matcher = backticksPattern.matcher(string);
        while(matcher.find()) {
            match = matcher.group(1);
            matchCount = match != null ? match.length() : 0;
            if (matchCount > maxCount)
                maxCount = matchCount;
        }

        return maxCount;
    }



    public static String getLiteralSingleLineMarkdownStringEntry(String label, Object object, String def) {
        return "**" + label + "**: " + (object != null ? object.toString() : def) +  "  ";
    }

    public static String getSingleLineMarkdownStringEntry(String label, Object object, String def) {
        if (object != null)
            return "**" + label + "**: " + getMarkdownCodeForString(object.toString(), false) +  "  ";
        else
            return "**" + label + "**: " + def +  "  ";
    }

    public static String getMultiLineMarkdownStringEntry(String label, Object object, String def) {
        if (object != null)
            return "**" + label + "**:\n" + getMarkdownCodeForString(object.toString(), true) + "\n";
        else
            return "**" + label + "**: " + def + "\n";
    }

    public static String getLinkMarkdownString(String label, String url) {
        if (url != null)
            return "[" + label.replaceAll("]", "\\\\]") + "](" + url.replaceAll("\\)", "\\\\)") +  ")";
        else
            return label;
    }


    /** Check following for more info:
     * https://github.com/noties/Markwon/tree/v4.6.2/app-sample
     * https://noties.io/Markwon/docs/v4/recycler/
     * https://github.com/noties/Markwon/blob/v4.6.2/app-sample/src/main/java/io/noties/markwon/app/readme/ReadMeActivity.kt
     */
    public static Markwon getRecyclerMarkwonBuilder(Context context) {
        return Markwon.builder(context)
            .usePlugin(LinkifyPlugin.create(Linkify.EMAIL_ADDRESSES | Linkify.WEB_URLS))
            .usePlugin(new AbstractMarkwonPlugin() {
                @Override
                public void configureVisitor(@NonNull MarkwonVisitor.Builder builder) {
                    builder.on(FencedCodeBlock.class, (visitor, fencedCodeBlock) -> {
                        // we actually won't be applying code spans here, as our custom xml view will
                        // draw background and apply mono typeface
                        //
                        // NB the `trim` operation on literal (as code will have a new line at the end)
                        final CharSequence code = visitor.configuration()
                            .syntaxHighlight()
                            .highlight(fencedCodeBlock.getInfo(), fencedCodeBlock.getLiteral().trim());
                        visitor.builder().append(code);
                    });
                }

                @Override
                public void configureSpansFactory(@NonNull MarkwonSpansFactory.Builder builder) {
                    // Do not change color for night themes
                    if (!ThemeUtils.isNightModeEnabled(context)) {
                        builder
                            // set color for inline code
                            .setFactory(Code.class, (configuration, props) -> new Object[]{
                                new BackgroundColorSpan(ContextCompat.getColor(context, R.color.background_markdown_code_inline)),
                            });
                    }
                }
            })
            .build();
    }

    /** Check following for more info:
     * https://github.com/noties/Markwon/tree/v4.6.2/app-sample
     * https://github.com/noties/Markwon/blob/v4.6.2/app-sample/src/main/java/io/noties/markwon/app/samples/notification/NotificationSample.java
     */
    public static Markwon getSpannedMarkwonBuilder(Context context) {
        return Markwon.builder(context)
                .usePlugin(StrikethroughPlugin.create())
                .usePlugin(new AbstractMarkwonPlugin() {
                    @Override
                    public void configureSpansFactory(@NonNull MarkwonSpansFactory.Builder builder) {
                        builder
                            .setFactory(Emphasis.class, (configuration, props) -> new StyleSpan(Typeface.ITALIC))
                            .setFactory(StrongEmphasis.class, (configuration, props) -> new StyleSpan(Typeface.BOLD))
                            .setFactory(BlockQuote.class, (configuration, props) -> new QuoteSpan())
                            .setFactory(Strikethrough.class, (configuration, props) -> new StrikethroughSpan())
                            // NB! notification does not handle background color
                            .setFactory(Code.class, (configuration, props) -> new Object[]{
                                new BackgroundColorSpan(ContextCompat.getColor(context, R.color.background_markdown_code_inline)),
                                new TypefaceSpan("monospace"),
                                new AbsoluteSizeSpan(48)
                            })
                            // NB! both ordered and bullet list items
                            .setFactory(ListItem.class, (configuration, props) -> new BulletSpan());
                    }
                })
                .build();
    }

    public static Spanned getSpannedMarkdownText(Context context, String string) {
        if (context == null || string == null) return null;
        final Markwon markwon = getSpannedMarkwonBuilder(context);
        return markwon.toMarkdown(string);
    }

}
