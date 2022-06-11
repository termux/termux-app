package com.termux.app.plugin;

import android.app.Service;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.BadParcelableException;
import android.os.Binder;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.NetworkOnMainThreadException;
import android.os.ParcelFileDescriptor;
import android.os.Process;
import android.os.RemoteException;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.termux.app.TermuxService;
import com.termux.plugin_aidl.IPluginCallback;
import com.termux.plugin_aidl.IPluginService;
import com.termux.plugin_aidl.Task;
import com.termux.shared.android.BinderUtils;
import com.termux.shared.errors.Error;
import com.termux.shared.file.FileUtils;
import com.termux.shared.logger.Logger;
import com.termux.shared.net.socket.local.ILocalSocketManager;
import com.termux.shared.net.socket.local.LocalClientSocket;
import com.termux.shared.net.socket.local.LocalSocketManager;
import com.termux.shared.net.socket.local.LocalSocketRunConfig;
import com.termux.shared.net.socket.local.PeerCred;
import com.termux.shared.shell.command.ExecutionCommand;
import com.termux.shared.shell.command.runner.nativerunner.NativeShell;
import com.termux.shared.termux.plugins.TermuxPluginUtils;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileWriter;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * This is a bound service that can be used by plugins.
 * The available methods are defined in {@link com.termux.plugin_aidl.IPluginService}.
 */
public class PluginService extends Service
{
    private final static String LOG_TAG = "PluginService";
    
    
    // map of connected clients by PID
    private final Map<Integer, Plugin> mConnectedPlugins = Collections.synchronizedMap(new HashMap<>());
    private final PluginServiceBinder mBinder = new PluginServiceBinder();
    private TermuxService mTermuxService; // can be null if TermuxService gets temporarily destroyed
    private final ServiceConnection mTermuxServiceConnection = new ServiceConnection()
    {
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            mTermuxService = ((TermuxService.LocalBinder) service).service;
        }
        
        @Override
        public void onServiceDisconnected(ComponentName name) {
            mTermuxService = null;
        }
        
        @Override
        public void onBindingDied(ComponentName name) {
            mTermuxService = null;
            Logger.logError("Binding to TermuxService died"); // this should never happen, as TermuxService is in the same process
        }
        
        @Override
        public void onNullBinding(ComponentName name) {
            // this should never happen, as TermuxService returns its Binder
            Logger.logError("TermuxService onBind returned no Binder");
        }
    };
    private final Handler mMainThreadHandler = new Handler(Looper.getMainLooper());
    
    
    
    /**
     * Internal representation of a connected plugin for the service.
     */
    private class Plugin {
        int pid, uid;
        
        Map<Integer, NativeShell> tasks = Collections.synchronizedMap(new HashMap<>());
    
        @NonNull IPluginCallback callback;
        int cachedCallbackVersion;
        
        Plugin(int pid, int uid, @NonNull IPluginCallback callback) throws RemoteException {
            this.pid = pid;
            this.uid = uid;
            this.callback = callback;
            callback.asBinder().linkToDeath(() -> mConnectedPlugins.remove(pid), 0); // remove self when the callback binder dies
            cachedCallbackVersion = callback.getCallbackVersion();
        }
        
        
        @Override
        public int hashCode() {
            return pid + uid;
        }
    
        @Override
        public boolean equals(@Nullable Object obj) {
            if (!(obj instanceof Plugin)) return false;
            Plugin p = (Plugin) obj;
            return p.pid == pid && p.uid == uid;
        }
    }
    
    @Override
    public void onCreate() {
        super.onCreate();
        if (! bindService(new Intent(this, TermuxService.class), mTermuxServiceConnection, Context.BIND_AUTO_CREATE)) {
            Logger.logError("Could not bind to TermuxService");
        }
    }
    
    @Override
    public void onDestroy() {
        unbindService(mTermuxServiceConnection);
    }
    
    
    @Override
    public IBinder onBind(Intent intent) {
        // If TermuxConstants.PROP_ALLOW_EXTERNAL_APPS property to not set to "true", don't enable binding
        String errmsg = TermuxPluginUtils.checkIfAllowExternalAppsPolicyIsViolated(this, LOG_TAG);
        if (errmsg != null) {
            return null;
        }
        return mBinder;
    }
    
    /**
     * If not already, this creates an entry in the map of connected plugins for the current Binder client.
     */
    private void addClient(@NonNull IPluginCallback callback) {
        int pid = Binder.getCallingPid(), uid = Binder.getCallingPid();
        if (pid == Process.myPid()) return; // no client connected
        if (mConnectedPlugins.get(pid) != null) return; // client already in list
        try {
            mConnectedPlugins.put(pid, new Plugin(pid, uid, callback));
        } catch (RemoteException ignored) {} // RemoteException is thrown if the callback binder is already dead, plugin isn't added to the list
    }
    
    @NonNull
    private Plugin checkClient() throws IllegalStateException {
        Plugin p = mConnectedPlugins.get(Binder.getCallingPid());
        if (p == null)  {
            Logger.logDebug("client not in list");
            throw new IllegalStateException("Please call setCallbackBinder first");
        }
        return p;
    }
    
    
    private void atPluginDeath(@NonNull Plugin p, Runnable r) {
        try {
            p.callback.asBinder().linkToDeath(r::run, 0);
        } catch (RemoteException e) {
            r.run();
        }
    }
    
    
    private class PluginServiceBinder extends IPluginService.Stub {
    
        /**
         * Convenience function to throw an {@link IllegalArgumentException} if external apps aren't enabled.
         */
        private void externalAppsOrThrow() {
            // If TermuxConstants.PROP_ALLOW_EXTERNAL_APPS property to not set to "true", then throw exception
            String errmsg = TermuxPluginUtils.checkIfAllowExternalAppsPolicyIsViolated(PluginService.this, LOG_TAG);
            if (errmsg != null) {
                throw new IllegalArgumentException(errmsg);
            }
        }
        
        @NonNull
        private String pluginDirOrThrow() {
            String pdir = BinderUtils.getCallingPluginDir(mTermuxService);
            if (pdir == null) throw new NullPointerException("Could not get apps dir of calling package");
            return pdir;
        }
        
        @NonNull
        private String fileInPluginDirOrThrow(String name) {
            String pluginDir = pluginDirOrThrow();
            String filePath = FileUtils.getCanonicalPath(pluginDir + "/" +name, null);
            if (FileUtils.isPathInDirPath(filePath, pluginDir, true)) {
                return filePath;
            } else {
                throw new IllegalArgumentException("A plugin cannot access paths outside of the plugin directory: "+filePath);
            }
        }
        
        @Override
        public void setCallbackBinder(IPluginCallback callback) {
            externalAppsOrThrow();
            if (callback == null) throw new NullPointerException("Passed callback binder is null");
            addClient(callback);
        }
    
        @Override
        public void setCallbackService(String componentNameString) {
            externalAppsOrThrow();
            if (componentNameString == null) throw new NullPointerException("Passed componentName is null");
            
            String callerPackageName = BinderUtils.getCallerPackageNameOrNull(PluginService.this);
            if (callerPackageName == null) throw new NullPointerException("Caller package is null");
            
            ComponentName componentName = ComponentName.createRelative(callerPackageName, componentNameString);
            Intent callbackStartIntent = new Intent();
            callbackStartIntent.setComponent(componentName);
    
    
            final boolean[] bindingFinished = {false};
            final IBinder[] callbackBinder = new IBinder[] {null};
            
            ServiceConnection con = new ServiceConnection()
            {
                @Override
                public void onServiceConnected(ComponentName name, IBinder service) {
                    callbackBinder[0] = service;
                    bindingFinished[0] = true;
                    synchronized (callbackBinder) {
                        callbackBinder.notifyAll();
                    }
                }
                @Override
                public void onServiceDisconnected(ComponentName name) {
                    unbindService(this);
                }
                @Override
                public void onBindingDied(ComponentName name) {
                    unbindService(this);
                }
                @Override
                public void onNullBinding(ComponentName name) {
                    bindingFinished[0] = true;
                    synchronized (callbackBinder) {
                        callbackBinder.notifyAll();
                    }
                    unbindService(this);
                }
            };
            
            PluginService.this.bindService(callbackStartIntent, con, Context.BIND_ALLOW_OOM_MANAGEMENT);
            
            while (! bindingFinished[0]) {
                try {
                    synchronized (callbackBinder) {
                        callbackBinder.wait();
                    }
                }
                catch (InterruptedException ignored) {}
            }
            
            if (callbackBinder[0] == null) {
                throw new IllegalArgumentException("Could not bind callback service: "+componentNameString);
            }
            
            addClient(IPluginCallback.Stub.asInterface(callbackBinder[0]));
        }
        
        
        @Override
        public Task runTask(String commandPath, String[] arguments, ParcelFileDescriptor stdin, String workdir, String[] environment) {
            externalAppsOrThrow();
            if (commandPath == null) throw new NullPointerException("Passed commandPath is null");
            if (stdin == null) throw new NullPointerException("Passed stdin is null");
            Plugin p = checkClient();
            BinderUtils.enforceRunCommandPermission(PluginService.this);
            
            
            final Object sync = new Object();
            final RuntimeException[] ex = new RuntimeException[1];
            final boolean[] finished = {false};
            final NativeShell[] shell = new NativeShell[1];
            // create pipes
            final ParcelFileDescriptor[] out = new ParcelFileDescriptor[2];
            final ParcelFileDescriptor[] in = new ParcelFileDescriptor[2];
            
            ParcelFileDescriptor[] pipes;
            try {
                pipes = ParcelFileDescriptor.createPipe();
            }
            catch (IOException e) {
                try {
                    stdin.close();
                } catch (IOException ignored) {}
                throw new RuntimeException(e);
            }
            in[0] = pipes[0];
            out[0] = pipes[1];
            
            try {
                pipes = ParcelFileDescriptor.createPipe();
            }
            catch (IOException e) {
                try {
                    stdin.close();
                } catch (IOException ignored) {}
                try {
                    out[0].close();
                } catch (IOException ignored) {}
                try {
                    in[0].close();
                } catch (IOException ignored) {}
                throw new RuntimeException(e);
            }
            in[1] = pipes[0];
            out[1] = pipes[1];
            
            mMainThreadHandler.post(() -> {
                TermuxService s = mTermuxService;
                if (s == null) {
                    synchronized (sync) {
                        ex[0] = new IllegalStateException("Termux service unavailable");
                        finished[0] = true;
                        sync.notifyAll();
                        return;
                    }
                }
                try {
                    ExecutionCommand cmd = new ExecutionCommand();
                    cmd.executable = commandPath;
                    cmd.workingDirectory = workdir;
                    cmd.arguments = arguments;
                    cmd.stdinFD = stdin;
                    cmd.stdoutFD = out[0];
                    cmd.stderrFD = out[1];
                    /*
                    try {
                        ParcelFileDescriptor od = out[0].dup();
                        new Thread(() -> {
                            try {
                                BufferedWriter w = new BufferedWriter(new FileWriter(od.getFileDescriptor()));
                                w.write("test");
                                w.flush();
                                w.close();
                            } catch (Exception ignored) {ignored.printStackTrace();}
                        }).start();
                    }
                    catch (IOException e) {
                        e.printStackTrace();
                    }
                    
                     */
                    
                    shell[0] = s.executeNativeShell(cmd, environment, (exitCode, error) -> {
                        try {
                            Logger.logDebug("NativeShell", "exit: "+exitCode);
                            // TODO callback
                        } catch (Exception ignored) {}
                    });
                    p.tasks.put(shell[0].getPid(), shell[0]);
                    synchronized (sync) {
                        finished[0] = true;
                        sync.notifyAll();
                    }
                } catch (RuntimeException e) {
                    synchronized (sync) {
                        ex[0] = e;
                        finished[0] = true;
                        sync.notifyAll();
                    }
                }
            });
            
            while (! finished[0]) {
                synchronized (sync) {
                    try {
                        sync.wait();
                    }
                    catch (InterruptedException e) {
                        throw new IllegalStateException(e);
                    }
                }
            }
            // make sure to not leak file descriptors
            if (ex[0] != null) {
                try {
                    stdin.close();
                } catch (IOException ignored) {}
                try {
                    out[0].close();
                } catch (IOException ignored) {}
                try {
                    out[1].close();
                } catch (IOException ignored) {}
                try {
                    in[0].close();
                } catch (IOException ignored) {}
                try {
                    in[1].close();
                } catch (IOException ignored) {}
                throw ex[0];
            }
            
            Task t = new Task();
            t.stdout = in[0];
            t.stderr = in[1];
            t.pid = shell[0].getPid();
            return t;
        }
    
        
        @Override
        public void listenOnSocketFile(String name) {
            externalAppsOrThrow();
            if (name == null) throw new NullPointerException("Passed name is null");
            Plugin p = checkClient();
            
            String socketPath = fileInPluginDirOrThrow(name);
    
            
            LocalSocketRunConfig conf = new LocalSocketRunConfig(BinderUtils.getCallerPackageName(PluginService.this) + " socket: " + name, socketPath, new ILocalSocketManager()
            {
                @Nullable
                @Override
                public Thread.UncaughtExceptionHandler getLocalSocketManagerClientThreadUEH(@NonNull LocalSocketManager localSocketManager) {
                    return null;
                }
    
                @Override
                public void onError(@NonNull LocalSocketManager localSocketManager, @Nullable LocalClientSocket clientSocket, @NonNull Error error) {
                    Logger.logDebug("PluginService:ILocalSocketManager", "Error: "+error.getErrorLogString());
                }
    
                @Override
                public void onDisallowedClientConnected(@NonNull LocalSocketManager localSocketManager, @NonNull LocalClientSocket clientSocket, @NonNull Error error) {
                    PeerCred pc = clientSocket.getPeerCred();
                    Logger.logDebug("PluginService:ILocalSocketManager", "Connection from a disallowed UID on a plugin socket detected:\n UID: "+pc.uid+
                        "\n Name: "+pc.pname+ "\n Cmdline: "+pc.cmdline+ "\n pid: "+pc.pid);
                }
    
                @Override
                public void onClientAccepted(@NonNull LocalSocketManager localSocketManager, @NonNull LocalClientSocket clientSocket) {
                    try {
                        p.callback.socketConnection(name, ParcelFileDescriptor.fromFd(clientSocket.getFD()));
                    }
                    catch (RemoteException | BadParcelableException | IllegalArgumentException | IllegalStateException
                           | NullPointerException | SecurityException | UnsupportedOperationException | NetworkOnMainThreadException | IOException e) {
                        Logger.logStackTrace("PluginService:ILocalSocketManager", e);
                    }
                    try {
                        clientSocket.close(); // close the socket, the plugin received a dup of the fd
                    }
                    catch (IOException e) {
                        Logger.logStackTrace("PluginService:ILocalSocketManager", e);
                    }
                }
            });
            LocalSocketManager m = new LocalSocketManager(PluginService.this, conf);
            Error err = m.start();
            if (err != null) {
                throw new UnsupportedOperationException("Error: "+err.getErrorLogString());
            }
            
            atPluginDeath(p, m::stop);
        }
        
    
        @Override
        public ParcelFileDescriptor openFile(String name, String mode) {
            externalAppsOrThrow();
            if (name == null) throw new NullPointerException("Passed name is null");
            if (mode == null) throw new NullPointerException("Passed mode is null");
            checkClient();
            
            String filePath = fileInPluginDirOrThrow(name);
            
            Error err = FileUtils.createRegularFile("("+BinderUtils.getCallerPackageName(PluginService.this)+")", filePath, "rw-", true, false);
            if (err != null) {
                throw new UnsupportedOperationException(err.getErrorLogString());
            }
            try {
                return ParcelFileDescriptor.open(new File(filePath), ParcelFileDescriptor.parseMode(mode));
            }
            catch (FileNotFoundException e) {
                throw new UnsupportedOperationException(e);
            }
        }
    
    
    }
}
