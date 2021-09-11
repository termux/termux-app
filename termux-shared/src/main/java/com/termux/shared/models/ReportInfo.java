package com.termux.shared.models;

import com.termux.shared.markdown.MarkdownUtils;
import com.termux.shared.termux.AndroidUtils;

import java.io.Serializable;

/**
 * An object that stored info for {@link com.termux.shared.activities.ReportActivity}.
 */
public class ReportInfo implements Serializable {

    /** The user action that was being processed for which the report was generated. */
    public final String userAction;
    /** The internal app component that sent the report. */
    public final String sender;
    /** The report title. */
    public final String reportTitle;
    /** The markdown report text prefix. Will not be part of copy and share operations, etc. */
    public String reportStringPrefix;
    /** The markdown report text. */
    public String reportString;
    /** The markdown report text suffix. Will not be part of copy and share operations, etc. */
    public String reportStringSuffix;
    /** If set to {@code true}, then report, app and device info will be added to the report when
     * markdown is generated.
     */
    public final boolean addReportInfoHeaderToMarkdown;
    /** The timestamp for the report. */
    public final String reportTimestamp;

    /** The label for the report file to save if user selects menu_item_save_report_to_file. */
    public final String reportSaveFileLabel;
    /** The path for the report file to save if user selects menu_item_save_report_to_file. */
    public final String reportSaveFilePath;

    public ReportInfo(String userAction, String sender, String reportTitle, String reportStringPrefix,
                      String reportString, String reportStringSuffix, boolean addReportInfoHeaderToMarkdown,
                      String reportSaveFileLabel, String reportSaveFilePath) {
        this.userAction = userAction;
        this.sender = sender;
        this.reportTitle = reportTitle;
        this.reportStringPrefix = reportStringPrefix;
        this.reportString = reportString;
        this.reportStringSuffix = reportStringSuffix;
        this.addReportInfoHeaderToMarkdown = addReportInfoHeaderToMarkdown;
        this.reportSaveFileLabel = reportSaveFileLabel;
        this.reportSaveFilePath = reportSaveFilePath;
        this.reportTimestamp = AndroidUtils.getCurrentMilliSecondUTCTimeStamp();
    }

    /**
     * Get a markdown {@link String} for {@link ReportInfo}.
     *
     * @param reportInfo The {@link ReportInfo} to convert.
     * @return Returns the markdown {@link String}.
     */
    public static String getReportInfoMarkdownString(final ReportInfo reportInfo) {
        if (reportInfo == null) return "null";

        StringBuilder markdownString = new StringBuilder();

        if (reportInfo.addReportInfoHeaderToMarkdown) {
            markdownString.append("## Report Info\n\n");
            markdownString.append("\n").append(MarkdownUtils.getSingleLineMarkdownStringEntry("User Action", reportInfo.userAction, "-"));
            markdownString.append("\n").append(MarkdownUtils.getSingleLineMarkdownStringEntry("Sender", reportInfo.sender, "-"));
            markdownString.append("\n").append(MarkdownUtils.getSingleLineMarkdownStringEntry("Report Timestamp", reportInfo.reportTimestamp, "-"));
            markdownString.append("\n##\n\n");
        }

        markdownString.append(reportInfo.reportString);

        return markdownString.toString();
    }

}
