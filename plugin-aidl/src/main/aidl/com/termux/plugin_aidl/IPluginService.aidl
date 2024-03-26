package com.termux.plugin_aidl;

import android.os.ParcelFileDescriptor;
import android.app.PendingIntent;

import com.termux.plugin_aidl.IPluginCallback;
import com.termux.plugin_aidl.Task;

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
    * Binds the service with @{link android.content.Context#BIND_WAIVE_PRIORITY}, meaning the plugins needs other components for the system not to kill it.
    */
    const int PRIORITY_MIN = 0;
    
    /**
    * Uses no flags to bind the service.
    */
    const int PRIORITY_NORMAL = 1;
    
    /**
    * Binds the service with @{link android.content.Context#BIND_IMPORTANT}, which should give the plugin the same priority as Termux in terms of OOM killing.
    */
    const int PRIORITY_IMPORTANT = 2;
    
    /**
    * Binds the service with @{link android.content.Context#BIND_ABOVE_CLIENT}, which would make Termux get killed first on OOM.
    * This should be used if you really don't want the plugin killed while a program uses it, and your plugin has a small memory footprint.
    * Another solution for that is to make your own foreground service, but the undismissable notification could be annoying for users, so this is given as an option.
    */
    const int PRIORITY_MAX = 3;
    
    
    
    
    /**
    * This or {@link com.termux.plugin_aidl.IPluginService#setCallbackService} has to be called before any other method.
    * It initializes the internal representation of the connected plugin and sets the callback binder.
    */
    void setCallbackBinder(IPluginCallback callback) = 1;
    
    /**
    * This or {@link com.termux.plugin_aidl.IPluginService#setCallbackBinder} has to be called before any other method.
    * It initializes the internal representation of the connected plugin and sets the callback binder to the binder returned by the bound service.
    * You can call other methods shortly after getCallbackVersion has been called in the supplied service.
    * If it's too early the methods will throw an {@link IllegalStateException} which can be caught.
    * 
    * @param componentName This is the relative part of a component name string.
    * The package name is always taken from the calling binder package for security reasons.
    * @param priority This is the priority Termux should bind the service with. See the PRIORITY_* constants for more info.
    */
    void setCallbackService(String componentName, int priority) = 2;
    
    
    /**
    * Runs a command in a Termux task in the background.
    * stdin, commandPath and workdir are required parameters.<br>
    * When a Task exists, the {@link com.termux.plugin_aidl.IPluginCallback#taskFinished} callback gets invoked.
    *
    * @param commandPath The full absolute path of the command binary to run.
    * @param arguments The command line arguments for the command. The first argument should be the same as {@code commandPath} (this fill be the 0th argument).
    * @param stdin The {@link android.os.ParcelFileDescriptor} used for the standard input of the command. You can e.g. create a pipe and use the read end here.
    * @param workdir The working directory of the command, also as an absolute path.
    * @param environment Additional environment variables for the command. Can be {@code null}.
    * @return A {@link com.termux.plugin_aidl.Task} that represents the running Task. It contains pipes for reading stdout and stderr.
    */
    Task runTask(String commandPath, in String[] arguments, in ParcelFileDescriptor stdin, String workdir, in String[] environment) = 3;
    
    /**
    * Send a signal to a Task.
    * @param pid The pid of the Task to kill.
    * @param signal The signal number to use. Using {@code kill -l} in bash and other shells lists the signals with their numbers.
    * @return Returns true if there was a Task with this pid, false if there was none.
    */
    boolean signalTask(int pid, int signal) = 4;
    
    /**
    * This creates a socket file with name under {@link com.termux.shared.termux.TermuxConstants#TERMUX_PLUGINS_DIR_PATH}/&lt;package name of caller&gt;.
    * Connections are transferred to the plugin via the {@link com.termux.plugin_aidl.IPluginService#socketConnection} method.
    * 
    * @param name Name of the socket file.
    */
    void listenOnSocketFile(String name) = 5;
    
    
    /**
    * Opens a file under {@link com.termux.shared.termux.TermuxConstants#TERMUX_PLUGINS_DIR_PATH}/&lt;package name of caller&gt; with mode.
    * 
    * @param name Name of the file.
    * @þaram mode Mode to use.
    */
    ParcelFileDescriptor openFile(String name, String mode) = 6;
    
    
    
    
}
