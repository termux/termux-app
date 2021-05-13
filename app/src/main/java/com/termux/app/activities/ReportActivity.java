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
import com.termux.shared.termux.TermuxConstants;
import com.termux.shared.markdown.MarkdownUtils;
import com.termux.shared.interact.ShareUtils;
import com.termux.app.models.ReportInfo;

import org.commonmark.node.FencedCodeBlock;

import io.noties.markwon.Markwon;
import io.noties.markwon.recycler.MarkwonAdapter;
import io.noties.markwon.recycler.SimpleEntry;

public class ReportActivity extends AppCompatActivity {

    private static final String EXTRA_REPORT_INFO = "report_info";

    ReportInfo mReportInfo;
    String mReportMarkdownString;
    String mReportActivityMarkdownString;

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
        if (intent != null)
            bundle = intent.getExtras();
        else if (savedInstanceState != null)
            bundle = savedInstanceState;

        updateUI(bundle);

    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        setIntent(intent);

        if (intent != null)
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

        final MarkwonAdapter adapter = MarkwonAdapter.builderTextViewIsRoot(R.layout.markdown_adapter_node_default)
            .include(FencedCodeBlock.class, SimpleEntry.create(R.layout.markdown_adapter_node_code_block, R.id.code_text_view))
            .build();

        recyclerView.setLayoutManager(new LinearLayoutManager(this));
        recyclerView.setAdapter(adapter);


        generateReportActivityMarkdownString();
        adapter.setMarkdown(markwon, mReportActivityMarkdownString);
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
            if (mReportMarkdownString != null)
                ShareUtils.shareText(this, getString(R.string.title_report_text), mReportMarkdownString);
        } else if (id == R.id.menu_item_copy_report) {
            if (mReportMarkdownString != null)
                ShareUtils.copyTextToClipboard(this, mReportMarkdownString, null);
        }

        return false;
    }

    /**
     * Generate the markdown {@link String} to be shown in {@link ReportActivity}.
     */
    private void generateReportActivityMarkdownString() {
        mReportMarkdownString = ReportInfo.getReportInfoMarkdownString(mReportInfo);

        mReportActivityMarkdownString = "";
        if (mReportInfo.reportStringPrefix != null)
            mReportActivityMarkdownString += mReportInfo.reportStringPrefix;

        mReportActivityMarkdownString += mReportMarkdownString;

        if (mReportInfo.reportStringSuffix != null)
            mReportActivityMarkdownString += mReportInfo.reportStringSuffix;
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
