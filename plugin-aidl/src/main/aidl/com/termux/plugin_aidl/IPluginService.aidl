package com.termux.plugin_aidl;

import android.os.ParcelFileDescriptor;
import android.app.PendingIntent;

import com.termux.plugin_aidl.IPluginCallback;


/**
* All available methods in {@link com.termux.app.plugin.PluginService}.
* When modifying methods here, use the following scheme to maintain backwards compatibility:
* <ul>
* <li>Append the version code of the last app version where the method exists with the current method signature as a postfix to the method you want to modify.</li>
* <li>Also rename the method in {@link com.termux.app.plugin.PluginService.PluginServiceBinder}.</li>
* <li>create a method with the original name, a new transaction id and the updated method signature.
* You should always count the transaction id up from the last created method to prevent clashing with old methods.</li>
* </ul>
* The old implementation should be kept for backwards compatibility, at least for some time.<br>
* Example:<br>
* Version code of the last app release: 118<br>
* Method: String foo(String bar) = 33;<br>
* Renamed: String foo_118(String bar) = 33;<br>
* New method: boolean foo(String bar) = 34;<br><br>
*/
interface IPluginService {
    
    
    /**
    * This or {@link com.termux.plugin_aidl.IPluginService#setCallbackService} has to be called before any other method.
    * It initializes the internal representation of the connected plugin and sets the callback binder.
    */
    void setCallbackBinder(IPluginCallback callback) = 1;
    
    /**
    * This or {@link com.termux.plugin_aidl.IPluginService#setCallbackBinder} has to be called before any other method.
    * It initialized the internal representation of the connected plugin and sets the callback binder to the binder returned by the bound service.
    * 
    * @param componentName This is the relative part of a component name string.
    * The package name is always taken from the calling binder package for security reasons.
    */
    void setCallbackService(String componentName) = 2;
    
    
    /**
    * Runs a command like through a RUN_COMMAND intent.
    * For documentation of the parameters, see <a href="https://github.com/termux/termux-app/wiki/RUN_COMMAND-Intent#run_command-intent-command-extras">the wiki</a>.
    * If a parameter is null it is treated the same as if the extra isn't in the intent.
    * <br><br>
    * This method runs synchronously and returns stout in [0] of the result array and stderr in [1].
    */
    ParcelFileDescriptor[] runTask(String commandPath, in String[] arguments, in ParcelFileDescriptor stdin, String workdir, String commandLabel, String commandDescription, String commandHelp) = 3;
    
    /**
    * This creates a socket file with name under {@link TermuxConstants#TERMUX_APPS_DIR_PATH}/plugins/&lt;package name of caller&gt; and listens on it.
    * 
    * @param name Name of the socket file.
    * @return The file descriptor of the created local server socket.
    */
    ParcelFileDescriptor listenOnSocketFile(String name) = 4;
    
    
    /**
    * Opens a file under{@link TermuxConstants#TERMUX_APPS_DIR_PATH}/plugins/&lt;package name of caller&gt; with mode.
    * 
    * @param name Name of the file.
    * @þaram mode Mode to use.
    */
    ParcelFileDescriptor openFile(String name, String mode) = 5;
    
    
    
    
}
