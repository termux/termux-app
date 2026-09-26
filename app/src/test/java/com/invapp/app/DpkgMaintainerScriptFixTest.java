package com.invapp.app;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import com.invapp.shared.termux.shell.command.environment.TermuxShellEnvironment;

import org.junit.Test;

public class DpkgMaintainerScriptFixTest {

    private static final String PREFIX =
        "/data/data/com.involvex.termux_app/files/usr";

    @Test
    public void rewritesComTermuxShebang() {
        String in = "#!/data/data/com.termux/files/usr/bin/bash\n"
            + "echo ok\n";
        String out = TermuxShellEnvironment.patchDpkgMaintainerScriptText(in, PREFIX);
        assertTrue(out.startsWith("#!/data/data/com.involvex.termux_app/files/usr/bin/bash\n"));
        assertTrue(out.contains("echo ok"));
    }

    @Test
    public void prependsShebangWhenMissing() {
        String in = "if [ \"$#\" = \"3\" ]; then\n"
            + "  exit 0\n"
            + "fi\n";
        String out = TermuxShellEnvironment.patchDpkgMaintainerScriptText(in, PREFIX, "preinst");
        assertTrue(out.startsWith("#!" + PREFIX + "/bin/sh\nif ["));
    }

    @Test
    public void rewritesBinShShebang() {
        String in = "#!/bin/sh\n"
            + "exit 0\n";
        String out = TermuxShellEnvironment.patchDpkgMaintainerScriptText(in, PREFIX);
        assertEquals("#!" + PREFIX + "/bin/sh\n"
            + "exit 0\n", out);
    }
}
