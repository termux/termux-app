package com.termux.app;

import android.app.Application;
import android.content.Context;
import android.content.Intent;

import androidx.appcompat.app.AppCompatDelegate;
import androidx.core.os.LocaleListCompat;

import com.termux.R;
import com.termux.shared.shell.command.ExecutionCommand;
import com.termux.shared.termux.TermuxConstants.TERMUX_APP.RUN_COMMAND_SERVICE;
import com.termux.shared.termux.TermuxConstants.TERMUX_APP.TERMUX_SERVICE;
import com.termux.shared.termux.plugins.TermuxPluginUtils;
import com.termux.shared.termux.settings.TermuxAppLocaleUtils;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.util.ReflectionHelpers;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

/** Real service error paths, with only result delivery/policy isolated (no shell or file writes). */
@RunWith(RobolectricTestRunner.class)
@Config(application = Application.class, sdk = {28, 32}, qualifiers = "en",
    shadows = ServiceLocaleRoutingTest.PluginResults.class)
public class ServiceLocaleRoutingTest {

    @Implements(value = TermuxPluginUtils.class, isInAndroidSdk = false)
    public static class PluginResults {
        static ExecutionCommand command;
        static String policyError;
        static boolean policyChecked;

        @Implementation
        protected static void processPluginExecutionCommandError(Context context, String logTag,
                ExecutionCommand executionCommand, boolean forceNotification) {
            command = executionCommand;
        }

        @Implementation
        protected static String checkIfAllowExternalAppsPolicyIsViolated(Context context, String logTag) {
            policyChecked = true;
            return policyError;
        }
    }

    @Before
    public void selectArabicOnEnglishDevice() {
        TermuxAppLocaleTest.resetAppCompat(Runnable::run);
        AppCompatDelegate.setApplicationLocales(LocaleListCompat.forLanguageTags("ar"));
        PluginResults.command = null;
        PluginResults.policyError = null;
        PluginResults.policyChecked = false;
    }

    private void assertError(Context service, int resourceId, String argument) {
        assertNotNull(PluginResults.command);
        assertEquals(TermuxAppLocaleUtils.getLocalizedContext(service).getString(resourceId, argument),
            PluginResults.command.resultData.errorsList.get(0).getMessage());
        assertTrue(PluginResults.command.resultData.errorsList.get(0).getMessage().matches("(?s).*[\\u0600-\\u06ff].*"));
    }

    @Test
    public void runCommandInvalidActionAndApiHelpUseArabicWithoutAnActivity() {
        RunCommandService service = Robolectric.buildService(RunCommandService.class).get();
        service.onStartCommand(new Intent("invalid.action"), 0, 1);
        assertError(service, R.string.error_run_command_service_invalid_intent_action, "invalid.action");
        assertEquals(TermuxAppLocaleUtils.getLocalizedContext(service).getString(
            R.string.error_run_command_service_api_help, RUN_COMMAND_SERVICE.RUN_COMMAND_API_HELP_URL),
            PluginResults.command.pluginAPIHelp);
    }

    @Test
    public void runCommandInvalidRunnerUsesArabic() {
        RunCommandService service = Robolectric.buildService(RunCommandService.class).get();
        service.onStartCommand(new Intent(RUN_COMMAND_SERVICE.ACTION_RUN_COMMAND)
            .putExtra(RUN_COMMAND_SERVICE.EXTRA_RUNNER, "invalid.runner"), 0, 1);
        assertError(service, R.string.error_run_command_service_invalid_execution_command_runner, "invalid.runner");
    }

    @Test
    public void runCommandMissingExecutableUsesArabicAfterAuthorization() {
        RunCommandService service = Robolectric.buildService(RunCommandService.class).get();
        service.onStartCommand(new Intent(RUN_COMMAND_SERVICE.ACTION_RUN_COMMAND), 0, 1);
        assertTrue(PluginResults.policyChecked);
        assertError(service, R.string.error_run_command_service_mandatory_extra_missing, RUN_COMMAND_SERVICE.EXTRA_COMMAND_PATH);
    }

    @Test
    public void deniedCommandStillCannotPopulateFileResultsBeforeAuthorization() {
        PluginResults.policyError = "policy denied";
        RunCommandService service = Robolectric.buildService(RunCommandService.class).get();
        service.onStartCommand(new Intent(RUN_COMMAND_SERVICE.ACTION_RUN_COMMAND)
            .putExtra(RUN_COMMAND_SERVICE.EXTRA_RESULT_DIRECTORY, "/must/not/be/used")
            .putExtra(RUN_COMMAND_SERVICE.EXTRA_RESULT_FILE_ERROR_FORMAT, "untrusted content"), 0, 1);
        assertTrue(PluginResults.policyChecked);
        assertEquals("policy denied", PluginResults.command.resultData.errorsList.get(0).getMessage());
        assertNull(PluginResults.command.resultConfig.resultDirectoryPath);
        assertNull(PluginResults.command.resultConfig.resultFileErrorFormat);
    }

    @Test
    public void termuxServiceInvalidRunnerUsesArabic() {
        TermuxService service = Robolectric.buildService(TermuxService.class).get();
        ReflectionHelpers.callInstanceMethod(service, "actionServiceExecute",
            ReflectionHelpers.ClassParameter.from(Intent.class, new Intent(TERMUX_SERVICE.ACTION_SERVICE_EXECUTE)
                .putExtra(TERMUX_SERVICE.EXTRA_RUNNER, "invalid.runner")));
        assertError(service, R.string.error_termux_service_invalid_execution_command_runner, "invalid.runner");
    }

    @Test
    public void termuxServiceShellCreateModeErrorsUseArabic() {
        TermuxService service = Robolectric.buildService(TermuxService.class).get();
        ExecutionCommand command = new ExecutionCommand();
        command.shellCreateMode = ExecutionCommand.ShellCreateMode.NO_SHELL_WITH_NAME.getMode();
        ReflectionHelpers.callInstanceMethod(service, "processShellCreateMode",
            ReflectionHelpers.ClassParameter.from(ExecutionCommand.class, command));
        assertError(service, R.string.error_termux_service_execution_command_shell_name_unset, command.shellCreateMode);
        command = new ExecutionCommand();
        command.shellCreateMode = "invalid.mode";
        ReflectionHelpers.callInstanceMethod(service, "processShellCreateMode",
            ReflectionHelpers.ClassParameter.from(ExecutionCommand.class, command));
        assertError(service, R.string.error_termux_service_unsupported_execution_command_shell_create_mode, "invalid.mode");
    }
}
