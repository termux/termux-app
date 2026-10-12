package com.termux.app.kyzz;

import com.termux.app.kyzz.appearance.KyzzAppearanceManager;
import com.termux.app.kyzz.editor.KyzzCodeEditorView;
import com.termux.app.kyzz.fonts.KyzzFontManager;
import com.termux.app.kyzz.modules.KyzzModuleItem;
import com.termux.app.kyzz.modules.KyzzModuleRegistry;
import com.termux.app.kyzz.packageadapter.KyzzGitAdapter;
import com.termux.app.kyzz.packageadapter.KyzzNpmAdapter;
import com.termux.app.kyzz.packageadapter.KyzzPipAdapter;
import com.termux.app.kyzz.packageadapter.KyzzPkgAdapter;
import com.termux.app.kyzz.sessions.KyzzSessionInfo;
import com.termux.app.kyzz.workspace.KyzzWorkspaceManager;

import org.junit.Assert;
import org.junit.Test;

import java.io.File;
import java.io.IOException;
import java.util.List;

public class KyzzTermFeaturesTest {

    @Test
    public void testPkgAdapterCommandConstruction() {
        KyzzPkgAdapter adapter = new KyzzPkgAdapter();
        Assert.assertEquals("pkg", adapter.getName());
        Assert.assertEquals("pkg install -y python", adapter.buildInstallCommand("python"));
        Assert.assertEquals("pkg uninstall -y nodejs", adapter.buildRemoveCommand("nodejs"));
        Assert.assertEquals("pkg upgrade -y git", adapter.buildUpdateCommand("git"));
        // Sanitize dangerous inputs
        Assert.assertEquals("pkg install -y python3", adapter.buildInstallCommand("python3; rm -rf /"));
    }

    @Test
    public void testNpmAdapterCommandConstruction() {
        KyzzNpmAdapter adapter = new KyzzNpmAdapter();
        Assert.assertEquals("npm", adapter.getName());
        Assert.assertEquals("npm install -g typescript", adapter.buildInstallCommand("typescript"));
        Assert.assertEquals("npm uninstall -g typescript", adapter.buildRemoveCommand("typescript"));
        Assert.assertEquals("npm update -g typescript", adapter.buildUpdateCommand("typescript"));
    }

    @Test
    public void testPipAdapterCommandConstruction() {
        KyzzPipAdapter adapter = new KyzzPipAdapter();
        Assert.assertEquals("pip", adapter.getName());
        Assert.assertEquals("pip install requests", adapter.buildInstallCommand("requests"));
        Assert.assertEquals("pip uninstall -y requests", adapter.buildRemoveCommand("requests"));
        Assert.assertEquals("pip install --upgrade requests", adapter.buildUpdateCommand("requests"));
    }

    @Test
    public void testGitAdapterCommandConstruction() {
        KyzzGitAdapter adapter = new KyzzGitAdapter();
        Assert.assertEquals("git", adapter.getName());
        Assert.assertEquals("git clone https://github.com/termux/termux-app",
                adapter.buildInstallCommand("https://github.com/termux/termux-app"));
        Assert.assertEquals("rm -rf myrepo", adapter.buildRemoveCommand("myrepo"));
        Assert.assertEquals("git -C /data/workspace/project pull",
                adapter.buildUpdateCommand("/data/workspace/project"));
    }

    @Test
    public void testModuleRegistryData() {
        List<KyzzModuleItem> modules = KyzzModuleRegistry.getDefaultModules("/non_existent_prefix");
        Assert.assertNotNull(modules);
        Assert.assertTrue(modules.size() > 10);

        boolean foundGit = false;
        boolean foundNode = false;
        boolean foundPython = false;

        for (KyzzModuleItem item : modules) {
            Assert.assertNotNull(item.getName());
            Assert.assertNotNull(item.getCategory());
            Assert.assertNotNull(item.getInstallCommand());
            if ("git".equals(item.getName())) foundGit = true;
            if ("nodejs".equals(item.getName())) foundNode = true;
            if ("python".equals(item.getName())) foundPython = true;
        }

        Assert.assertTrue(foundGit);
        Assert.assertTrue(foundNode);
        Assert.assertTrue(foundPython);
    }

    @Test
    public void testSessionInfo() {
        KyzzSessionInfo session = new KyzzSessionInfo("sess_1", "Main Terminal", "/home", true);
        Assert.assertEquals("sess_1", session.getHandle());
        Assert.assertEquals("Main Terminal", session.getName());
        Assert.assertEquals("/home", session.getCwd());
        Assert.assertTrue(session.isActive());

        session.setName("Dev Server");
        Assert.assertEquals("Dev Server", session.getName());
    }
}
