package com.termux.app.kyzz.packageadapter;

import java.io.File;

public class KyzzGitAdapter implements KyzzPackageManagerAdapter {
    @Override
    public String getName() {
        return "git";
    }

    @Override
    public boolean isAvailable(String prefixPath) {
        if (prefixPath == null) prefixPath = "/data/data/com.termux/files/usr";
        File git = new File(prefixPath + "/bin/git");
        return git.exists() && git.canExecute();
    }

    @Override
    public String buildInstallCommand(String repositoryUrl) {
        return "git clone " + sanitizeUrl(repositoryUrl);
    }

    @Override
    public String buildRemoveCommand(String repoName) {
        return "rm -rf " + sanitize(repoName);
    }

    @Override
    public String buildUpdateCommand(String repoPath) {
        return "git -C " + sanitize(repoPath) + " pull";
    }

    private String sanitizeUrl(String url) {
        if (url == null) return "";
        return url.replaceAll("[^a-zA-Z0-9_.:/\\-@]", "");
    }

    private String sanitize(String input) {
        if (input == null) return "";
        return input.replaceAll("[^a-zA-Z0-9_.-]", "");
    }
}
