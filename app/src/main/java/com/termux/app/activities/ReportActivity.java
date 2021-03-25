package com.termux.app.activities;

import androidx.annotation.NonNull;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;

import com.termux.R;
import com.termux.app.TermuxConstants;
import com.termux.app.utils.MarkdownUtils;
import com.termux.app.utils.ShareUtils;
import com.termux.app.utils.TermuxUtils;
import com.termux.models.ReportInfo;

import org.commonmark.node.FencedCodeBlock;

import io.noties.markwon.Markwon;
import io.noties.markwon.recycler.MarkwonAdapter;
import io.noties.markwon.recycler.SimpleEntry;

public class ReportActivity extends AppCompatActivity {

    private static final String EXTRA_REPORT_INFO = "report_info";

    ReportInfo mReportInfo;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_report);

        Toolbar toolbar = findViewById(R.id.toolbar);
        if (toolbar != null) {
            setSupportActionBar(toolbar);
        }

        Bundle bundle = null;
        Intent intent = getIntent();
        if(intent != null)
            bundle = intent.getExtras();
        else if(savedInstanceState != null)
            bundle = savedInstanceState;

        updateUI(bundle);

    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        setIntent(intent);

        if(intent != null)
            updateUI(intent.getExtras());
    }

    private void updateUI(Bundle bundle) {

        if (bundle == null) {
            finish();
            return;
        }

        mReportInfo = (ReportInfo) bundle.getSerializable(EXTRA_REPORT_INFO);

        if (mReportInfo == null) {
            finish();
            return;
        }


        final ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            if (mReportInfo.reportTitle != null)
                actionBar.setTitle(mReportInfo.reportTitle);
            else
                actionBar.setTitle(TermuxConstants.TERMUX_APP_NAME + " App Report");
        }


        RecyclerView recyclerView = findViewById(R.id.recycler_view);

        final Markwon markwon = MarkdownUtils.getRecyclerMarkwonBuilder(this);

        final MarkwonAdapter adapter = MarkwonAdapter.builderTextViewIsRoot(R.layout.activity_report_adapter_node_default)
            .include(FencedCodeBlock.class, SimpleEntry.create(R.layout.activity_report_adapter_node_code_block, R.id.code_text_view))
            .build();

        recyclerView.setLayoutManager(new LinearLayoutManager(this));
        recyclerView.setAdapter(adapter);

        adapter.setMarkdown(markwon, mReportInfo.reportString + getReportAndDeviceDetailsMarkdownString());
        adapter.notifyDataSetChanged();
    }



    @Override
    public void onSaveInstanceState(@NonNull Bundle outState) {
        super.onSaveInstanceState(outState);

        outState.putSerializable(EXTRA_REPORT_INFO, mReportInfo);
    }

    @Override
    public boolean onCreateOptionsMenu(final Menu menu) {
        final MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.menu_report, menu);
        return true;
    }

    @Override
    public void onBackPressed() {
        // Remove activity from recents menu on back button press
        finishAndRemoveTask();
    }

    @Override
    public boolean onOptionsItemSelected(final MenuItem item) {
        int id = item.getItemId();
        if (id == R.id.menu_item_share_report) {
            if (mReportInfo != null)
                ShareUtils.shareText(this, getString(R.string.title_report_text), mReportInfo.reportString);
        } else if (id == R.id.menu_item_copy_report) {
            if (mReportInfo != null)
                ShareUtils.copyTextToClipboard(this, mReportInfo.reportString, null);
        }

        return false;
    }

    /**
     * Get a markdown {@link String} for {@link #mReportInfo} and device details.
     *
     * @return Returns the markdown {@link String}.
     */
    private String getReportAndDeviceDetailsMarkdownString() {
        if(!mReportInfo.addReportAndDeviceDetails) return "";

        StringBuilder markdownString = new StringBuilder();

        markdownString.append("\n\n### Report And Device Details\n\n");

        if (mReportInfo != null) {
            markdownString.append("\n").append(MarkdownUtils.getSingleLineMarkdownStringEntry("User Action", mReportInfo.userAction, "-"));
            markdownString.append("\n").append(MarkdownUtils.getSingleLineMarkdownStringEntry("Sender", mReportInfo.sender, "-"));
        }

        markdownString.append("\n").append(MarkdownUtils.getSingleLineMarkdownStringEntry("Timestamp", TermuxUtils.getCurrentTimeStamp(), "-"));

        markdownString.append("\n\n").append(TermuxUtils.getDeviceDetailsMarkdownString(this));

        markdownString.append("\n##\n");

        return markdownString.toString();
    }



    public static void startReportActivity(@NonNull final Context context, @NonNull final ReportInfo reportInfo) {
        context.startActivity(newInstance(context, reportInfo));
    }

    public static Intent newInstance(@NonNull final Context context, @NonNull final ReportInfo reportInfo) {
        Intent intent = new Intent(context, ReportActivity.class);
        Bundle bundle = new Bundle();
        bundle.putSerializable(EXTRA_REPORT_INFO, reportInfo);
        intent.putExtras(bundle);

        // Note that ReportActivity task has documentLaunchMode="intoExisting" set in AndroidManifest.xml
        // which has equivalent behaviour to the following. The following dynamic way doesn't seem to
        // work for notification pending intent, i.e separate task isn't created and activity is
        // launched in the same task as TermuxActivity.
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK |  Intent.FLAG_ACTIVITY_NEW_DOCUMENT);
        return intent;
    }

}
