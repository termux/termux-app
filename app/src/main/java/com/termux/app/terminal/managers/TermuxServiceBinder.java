package com.termux.app.terminal.managers;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.IBinder;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.termux.app.TermuxService;
import com.termux.shared.logger.Logger;

/**
 * Manages service binding for TermuxActivity.
 * Follows Single Responsibility Principle by handling only service binding operations.
 */
public class TermuxServiceBinder implements ServiceConnection {
    
    private static final String LOG_TAG = "TermuxServiceBinder";
    
    private final Activity mActivity;
    private TermuxService mTermuxService;
    private ServiceBindingListener mListener;
    
    /**
     * Interface for service binding callbacks.
     */
    public interface ServiceBindingListener {
        void onServiceConnected(TermuxService service);
        void onServiceDisconnected();
    }
    
    public TermuxServiceBinder(@NonNull Activity activity) {
        this.mActivity = activity;
    }
    
    /**
     * Set the service binding listener.
     */
    public void setServiceBindingListener(ServiceBindingListener listener) {
        this.mListener = listener;
    }
    
    /**
     * Start and bind to the Termux service.
     */
    public void bindService() {
        Logger.logVerbose(LOG_TAG, "Binding to TermuxService");
        
        // Start the service
        Intent serviceIntent = new Intent(mActivity, TermuxService.class);
        
        // Start service as foreground service
        mActivity.startService(serviceIntent);
        
        // Bind to the service
        if (!mActivity.bindService(serviceIntent, this, Context.BIND_AUTO_CREATE)) {
            Logger.logError(LOG_TAG, "Failed to bind to TermuxService");
            throw new RuntimeException("Failed to bind to TermuxService");
        }
    }
    
    /**
     * Unbind from the Termux service.
     */
    public void unbindService() {
        if (mTermuxService != null) {
            Logger.logVerbose(LOG_TAG, "Unbinding from TermuxService");
            
            try {
                mActivity.unbindService(this);
            } catch (Exception e) {
                Logger.logError(LOG_TAG, "Error unbinding from service: " + e.getMessage());
            }
            
            mTermuxService = null;
            
            if (mListener != null) {
                mListener.onServiceDisconnected();
            }
        }
    }
    
    /**
     * Get the bound Termux service.
     */
    @Nullable
    public TermuxService getTermuxService() {
        return mTermuxService;
    }
    
    /**
     * Check if service is bound.
     */
    public boolean isServiceBound() {
        return mTermuxService != null;
    }
    
    @Override
    public void onServiceConnected(ComponentName componentName, IBinder service) {
        Logger.logVerbose(LOG_TAG, "TermuxService connected");
        
        mTermuxService = ((TermuxService.LocalBinder) service).service;
        
        if (mListener != null) {
            mListener.onServiceConnected(mTermuxService);
        }
    }
    
    @Override
    public void onServiceDisconnected(ComponentName componentName) {
        Logger.logVerbose(LOG_TAG, "TermuxService disconnected");
        
        mTermuxService = null;
        
        if (mListener != null) {
            mListener.onServiceDisconnected();
        }
    }
    
    /**
     * Stop the service if no sessions are running.
     */
    public void stopServiceIfNoSessions() {
        if (mTermuxService != null && mTermuxService.getSessions().isEmpty()) {
            Logger.logVerbose(LOG_TAG, "Stopping TermuxService - no sessions running");
            
            // Unbind first
            unbindService();
            
            // Stop the service
            Intent serviceIntent = new Intent(mActivity, TermuxService.class);
            mActivity.stopService(serviceIntent);
        }
    }
    
    /**
     * Request service to update notification.
     */
    public void updateNotification() {
        if (mTermuxService != null) {
            mTermuxService.updateNotification();
        }
    }
    
    /**
     * Check if service should be kept alive.
     */
    public boolean shouldKeepServiceAlive() {
        if (mTermuxService == null) {
            return false;
        }
        
        // Keep service alive if there are sessions or background tasks
        return !mTermuxService.getSessions().isEmpty() || 
               mTermuxService.hasBackgroundTasks();
    }
}