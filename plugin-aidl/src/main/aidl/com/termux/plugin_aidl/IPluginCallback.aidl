package com.termux.plugin_aidl;


import android.os.ParcelFileDescriptor;

/**
* This can be passed from a plugin to Termux to enable callback functionality.
* To maintain backwards compatibility callbacks cannot be removed (without deprecation).
* New callbacks can be added and CURRENT_CALLBACK_VERSION incremented, but the callbacks can only be called
* when the callback version via getCallbackVersion() is at least the new value of CURRENT_CALLBACK_VERSION.
* Previous callback versions have to be supported by Termux, so an out-of-date plugin doesn't receive transactions it doesn't recognize.
* In the javadoc write the first and last (if there is one) callback version code where the callback will be called.
*/
interface IPluginCallback {
    
    /**
    * This defines to most up-to-date version code for callback binders.
    */
    const int CURRENT_CALLBACK_VERSION = 1;
    
    /**
    * Returns the callback version supported by this Binder.
    * This is the first method called after Termux receives the Binder.
    * Only methods compatible with the callback version will be called.
    */
    int getCallbackVersion() = 1;
    
    /**
    * This gets called when a connection is made on a socket created with {@link com.termux.plugin_aidl.IPluginService#listenOnSocketFile}.
    * <br>Added in version 1.
    * 
    * @param sockname The name of socket file the connection was made on (the relative path to the plugin directory).
    * @param connection The connection file descriptor.
    */
    void socketConnection(String sockname, in ParcelFileDescriptor connection) = 2;
    
    
    /**
    * Gets called when a started Task exits.
    * <br>Added in version 1.
    * 
    * @param pid The pid of the task that exited.
    * @param code The exit code of the Task. Negative values indicate the Task was killed by a signal. The signal number is then {@code -code}.
    */
    void taskFinished(int pid, int code) = 3;
    
    
    
    
    
    
    
}
