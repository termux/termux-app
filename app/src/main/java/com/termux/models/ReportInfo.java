package com.termux.models;

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
    /** If set to {@code true}, then report and device details will be added to the report. */
    public boolean addReportAndDeviceDetails;

    public ReportInfo(UserAction userAction, String sender, String reportTitle, String reportString, boolean addReportAndDeviceDetails) {
        this.userAction = userAction;
        this.sender = sender;
        this.reportTitle = reportTitle;
        this.reportString = reportString;
        this.addReportAndDeviceDetails = addReportAndDeviceDetails;
    }

}
