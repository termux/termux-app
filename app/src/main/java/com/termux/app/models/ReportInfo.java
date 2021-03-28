package com.termux.app.models;

import android.content.Context;

import com.termux.app.utils.MarkdownUtils;
import com.termux.app.utils.TermuxUtils;

import java.io.Serializable;

public class ReportInfo implements Serializable {

    /** The user action that was being processed for which the report was generated. */
    public UserAction userAction;
    /** The internal app component that sent the report. */
    public String sender;
    /** The report title. */
    public String reportTitle;
    /** The markdown text for the report. */
    public String reportString;
    /** If set to {@code true}, then report, app and device info will be added to the report when
     * markdown is generated.
     */
    public boolean addReportInfoToMarkdown;
    /** The timestamp for the report. */
    public String creationTimestammp;

    public ReportInfo(UserAction userAction, String sender, String reportTitle, String reportString, boolean addReportInfoToMarkdown) {
        this.userAction = userAction;
        this.sender = sender;
        this.reportTitle = reportTitle;
        this.reportString = reportString;
        this.addReportInfoToMarkdown = addReportInfoToMarkdown;
        this.creationTimestammp = TermuxUtils.getCurrentTimeStamp();
    }

    /**
     * Get a markdown {@link String} for {@link ReportInfo}.
     *
     * @param currentPackageContext The context of current package.
     * @param reportInfo The {@link ReportInfo} to convert.
     * @return Returns the markdown {@link String}.
     */
    public static String getReportInfoMarkdownString(final Context currentPackageContext, final ReportInfo reportInfo) {
        if (reportInfo == null) return "null";

        StringBuilder markdownString = new StringBuilder();

        markdownString.append(reportInfo.reportString);

        if(reportInfo.addReportInfoToMarkdown) {
            markdownString.append("## Report Info\n\n");
            markdownString.append("\n").append(MarkdownUtils.getSingleLineMarkdownStringEntry("User Action", reportInfo.userAction, "-"));
            markdownString.append("\n").append(MarkdownUtils.getSingleLineMarkdownStringEntry("Sender", reportInfo.sender, "-"));
            markdownString.append("\n").append(MarkdownUtils.getSingleLineMarkdownStringEntry("Creation Timestamp", reportInfo.creationTimestammp, "-"));
            markdownString.append("\n##\n");

            markdownString.append("\n\n").append(TermuxUtils.getAppInfoMarkdownString(currentPackageContext, true));
            markdownString.append("\n\n").append(TermuxUtils.getDeviceInfoMarkdownString(currentPackageContext));
        }

        return markdownString.toString();
    }

}
