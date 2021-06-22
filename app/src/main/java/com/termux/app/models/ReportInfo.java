package com.termux.app.models;

import com.termux.shared.markdown.MarkdownUtils;
import com.termux.shared.termux.TermuxUtils;

import java.io.Serializable;

public class ReportInfo implements Serializable {

    /** The user action that was being processed for which the report was generated. */
    public final UserAction userAction;
    /** The internal app component that sent the report. */
    public final String sender;
    /** The report title. */
    public final String reportTitle;
    /** The markdown report text prefix. Will not be part of copy and share operations, etc. */
    public final String reportStringPrefix;
    /** The markdown report text. */
    public final String reportString;
    /** The markdown report text suffix. Will not be part of copy and share operations, etc. */
    public final String reportStringSuffix;
    /** If set to {@code true}, then report, app and device info will be added to the report when
     * markdown is generated.
     */
    public final boolean addReportInfoToMarkdown;
    /** The timestamp for the report. */
    public final String reportTimestamp;

    public ReportInfo(UserAction userAction, String sender, String reportTitle, String reportStringPrefix, String reportString, String reportStringSuffix, boolean addReportInfoToMarkdown) {
        this.userAction = userAction;
        this.sender = sender;
        this.reportTitle = reportTitle;
        this.reportStringPrefix = reportStringPrefix;
        this.reportString = reportString;
        this.reportStringSuffix = reportStringSuffix;
        this.addReportInfoToMarkdown = addReportInfoToMarkdown;
        this.reportTimestamp = TermuxUtils.getCurrentTimeStamp();
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

        if (reportInfo.addReportInfoToMarkdown) {
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
