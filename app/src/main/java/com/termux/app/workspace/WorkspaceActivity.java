package com.termux.app.workspace;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.provider.DocumentsContract;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.content.Context;
import android.text.TextUtils;
import android.widget.ArrayAdapter;
import android.widget.ImageButton;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;

import com.termux.R;
import com.termux.app.TermuxActivity;
import com.termux.app.TermuxService;
import com.termux.shared.activity.ActivityUtils;
import com.termux.shared.data.IntentUtils;
import com.termux.shared.termux.TermuxConstants;
import com.termux.shared.termux.TermuxConstants.TERMUX_APP.TERMUX_SERVICE;
import com.termux.shared.logger.Logger;

import java.io.File;
import java.util.List;

public class WorkspaceActivity extends AppCompatActivity {

    private static final int REQUEST_CODE_PICK_DIRECTORY = 1001;

    private WorkspaceManager mWorkspaceManager;
    private WorkspaceAdapter mAdapter;
    private ListView mListView;
    private View mEmptyView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_workspace);

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        if (getSupportActionBar() != null) {
            getSupportActionBar().setDisplayHomeAsUpEnabled(true);
            getSupportActionBar().setTitle(R.string.title_workspaces);
        }

        mWorkspaceManager = new WorkspaceManager(this);

        mListView = findViewById(R.id.workspace_list);
        mEmptyView = findViewById(R.id.empty_view);
        mListView.setEmptyView(mEmptyView);

        mAdapter = new WorkspaceAdapter(this, mWorkspaceManager.getWorkspaces());
        mListView.setAdapter(mAdapter);

        mListView.setOnItemClickListener((parent, view, position, id) -> {
            WorkspaceManager.Workspace workspace = mAdapter.getItem(position);
            if (workspace != null) {
                openWorkspaceInTerminal(workspace);
            }
        });

        mListView.setOnItemLongClickListener((parent, view, position, id) -> {
            WorkspaceManager.Workspace workspace = mAdapter.getItem(position);
            if (workspace != null) {
                showWorkspaceContextMenu(workspace, view);
                return true;
            }
            return false;
        });

        findViewById(R.id.fab_add_workspace).setOnClickListener(v -> pickDirectory());
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.menu_workspace, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        int id = item.getItemId();
        if (id == android.R.id.home) {
            finish();
            return true;
        } else if (id == R.id.action_add_workspace) {
            pickDirectory();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    private void pickDirectory() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
        intent.putExtra(DocumentsContract.EXTRA_INITIAL_URI, Uri.parse("file://" + TermuxConstants.TERMUX_HOME_DIR_PATH));
        startActivityForResult(intent, REQUEST_CODE_PICK_DIRECTORY);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == REQUEST_CODE_PICK_DIRECTORY && resultCode == Activity.RESULT_OK && data != null) {
            Uri treeUri = data.getData();
            if (treeUri != null) {
                // Persist URI permission
                getContentResolver().takePersistableUriPermission(treeUri,
                    Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);

                // Get path from URI
                String path = getPathFromTreeUri(treeUri);
                if (path != null) {
                    showNameDialog(path);
                } else {
                    Toast.makeText(this, "Failed to get directory path", Toast.LENGTH_SHORT).show();
                }
            }
        }
    }

    private String getPathFromTreeUri(Uri treeUri) {
        // For file:// URIs, extract the path
        if ("file".equals(treeUri.getScheme())) {
            return treeUri.getPath();
        }
        // For content:// URIs, we'd need to use DocumentsContract
        // For simplicity, return the path if it's a file URI
        return treeUri.getPath();
    }

    private void showNameDialog(String path) {
        File file = new File(path);
        String defaultName = file.getName();

        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle(R.string.action_add_workspace);

        final android.widget.EditText input = new android.widget.EditText(this);
        input.setHint(R.string.workspace_name_hint);
        input.setText(defaultName);
        builder.setView(input);

        builder.setPositiveButton(R.string.action_add_workspace, (dialog, which) -> {
            String name = input.getText().toString().trim();
            if (TextUtils.isEmpty(name)) {
                name = defaultName;
            }
            WorkspaceManager.Workspace ws = mWorkspaceManager.addWorkspace(name, path);
            mAdapter.updateWorkspaces(mWorkspaceManager.getWorkspaces());
            Toast.makeText(this, R.string.msg_workspace_added, Toast.LENGTH_SHORT).show();
        });

        builder.setNegativeButton(android.R.string.cancel, null);
        builder.show();
    }

    private void showWorkspaceContextMenu(WorkspaceManager.Workspace workspace, View anchor) {
        android.widget.PopupMenu popup = new android.widget.PopupMenu(this, anchor);
        popup.getMenuInflater().inflate(R.menu.menu_workspace_item, popup.getMenu());

        MenuItem favoriteItem = popup.getMenu().findItem(R.id.action_toggle_favorite);
        favoriteItem.setTitle(workspace.isFavorite ? getString(R.string.action_remove_favorite) : getString(R.string.action_set_as_favorite));

        MenuItem gitItem = popup.getMenu().findItem(R.id.action_git_status);
        gitItem.setVisible(workspace.isGitRepo);
        if (workspace.isGitRepo) {
            gitItem.setTitle(workspace.gitBranch != null ? "Git: " + workspace.gitBranch : getString(R.string.msg_git_detected));
        }

        popup.setOnMenuItemClickListener(item -> {
            int id = item.getItemId();
            if (id == R.id.action_open_in_terminal) {
                openWorkspaceInTerminal(workspace);
                return true;
            } else if (id == R.id.action_toggle_favorite) {
                mWorkspaceManager.toggleFavorite(workspace.path);
                mAdapter.updateWorkspaces(mWorkspaceManager.getWorkspaces());
                Toast.makeText(this, workspace.isFavorite ? "Removed from favorites" : "Added to favorites", Toast.LENGTH_SHORT).show();
                return true;
            } else if (id == R.id.action_remove_workspace) {
                new AlertDialog.Builder(this)
                    .setTitle("Remove workspace")
                    .setMessage("Remove \"" + workspace.getDisplayName() + "\"?")
                    .setPositiveButton(R.string.action_remove_workspace, (dialog, which) -> {
                        mWorkspaceManager.removeWorkspace(workspace.path);
                        mAdapter.updateWorkspaces(mWorkspaceManager.getWorkspaces());
                        Toast.makeText(this, R.string.msg_workspace_removed, Toast.LENGTH_SHORT).show();
                    })
                    .setNegativeButton(android.R.string.cancel, null)
                    .show();
                return true;
            } else if (id == R.id.action_refresh_git) {
                mWorkspaceManager.updateGitStatus(workspace.path);
                mAdapter.updateWorkspaces(mWorkspaceManager.getWorkspaces());
                Toast.makeText(this, "Git status refreshed", Toast.LENGTH_SHORT).show();
                return true;
            }
            return false;
        });

        popup.show();
    }

    private void openWorkspaceInTerminal(WorkspaceManager.Workspace workspace) {
        mWorkspaceManager.updateLastAccessed(workspace.path);
        mAdapter.updateWorkspaces(mWorkspaceManager.getWorkspaces());

        // Start TermuxActivity with the workspace directory as working directory
        Intent intent = new Intent(this, TermuxActivity.class);
        intent.putExtra(TERMUX_SERVICE.EXTRA_WORKDIR, workspace.path);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TOP);
        startActivity(intent);
        finish();
    }

    private static class WorkspaceAdapter extends ArrayAdapter<WorkspaceManager.Workspace> {
        private final LayoutInflater mInflater;
        private List<WorkspaceManager.Workspace> mWorkspaces;

        WorkspaceAdapter(Context context, List<WorkspaceManager.Workspace> workspaces) {
            super(context, 0, workspaces);
            mInflater = LayoutInflater.from(context);
            mWorkspaces = workspaces;
        }

        void updateWorkspaces(List<WorkspaceManager.Workspace> workspaces) {
            mWorkspaces = workspaces;
            clear();
            addAll(workspaces);
            notifyDataSetChanged();
        }

        @NonNull
        @Override
        public View getView(int position, @Nullable View convertView, @NonNull ViewGroup parent) {
            View view = convertView;
            if (view == null) {
                view = mInflater.inflate(R.layout.item_workspace, parent, false);
            }

            WorkspaceManager.Workspace workspace = getItem(position);
            if (workspace == null) return view;

            TextView nameView = view.findViewById(R.id.workspace_name);
            TextView pathView = view.findViewById(R.id.workspace_path);
            ImageButton favoriteBtn = view.findViewById(R.id.workspace_favorite);
            TextView gitBadge = view.findViewById(R.id.workspace_git_badge);

            nameView.setText(workspace.getDisplayName());
            pathView.setText(workspace.path);

            favoriteBtn.setImageResource(workspace.isFavorite ? R.drawable.ic_star_filled : R.drawable.ic_star_outline);
            favoriteBtn.setOnClickListener(v -> {
                // Handled by long-click context menu
            });

            if (workspace.isGitRepo) {
                gitBadge.setVisibility(View.VISIBLE);
                gitBadge.setText(workspace.gitBranch != null ? "🌿 " + workspace.gitBranch : "🌿 Git");
            } else {
                gitBadge.setVisibility(View.GONE);
            }

            return view;
        }
    }
}
