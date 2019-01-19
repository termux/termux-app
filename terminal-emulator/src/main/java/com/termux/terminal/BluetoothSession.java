package com.termux.terminal;


import android.annotation.SuppressLint;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.os.Handler;
import android.os.Message;
import android.util.Log;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.UUID;
import java.util.logging.Logger;

public class BluetoothSession extends TerminalSession {

    private String address = null;
    BluetoothAdapter myBluetooth = null;
    BluetoothSocket btSocket = null;
    DataInputStream mmInStream = null;
    DataOutputStream mmOutStream = null;

    //SPP UUID. Look for it
    static final UUID sppUUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");

    public BluetoothSession(String btAddresss, String cwd, String[] args, String[] env, SessionChangedCallback changeCallback) {
        super("", null, null, null, changeCallback);
        address = btAddresss;

        try {
            if (btSocket == null || !btSocket.isConnected()) {
                myBluetooth = BluetoothAdapter.getDefaultAdapter();  //get the mobile bluetooth device
                BluetoothDevice dispositivo = myBluetooth.getRemoteDevice(address); //connects to the device's address and checks if it's available
                btSocket = dispositivo.createInsecureRfcommSocketToServiceRecord(sppUUID); //create a RFCOMM (SPP) connection
                BluetoothAdapter.getDefaultAdapter().cancelDiscovery();
                btSocket.connect();//start connection
            } else {
                byte[] error = "Cannot connect to device\n".getBytes();
                mEmulator.append(error, error.length);
                disconnect();
            }
        } catch (Exception e) {
            byte[] error = (e.getMessage() + "\n").getBytes();
            mEmulator.append(error, error.length);
            disconnect();
        }
    }

    @Override
    public void initializeEmulator(int columns, int rows) {
        mEmulator = new TerminalEmulator(this, columns, rows, /* transcript= */2000);

        new Thread("BluetoothSession[uuid=" + address + "]") {
            @Override
            public void run() {

                if (btSocket.isConnected()) try {
                    mmInStream = new DataInputStream(btSocket.getInputStream());
                    mmOutStream = new DataOutputStream(btSocket.getOutputStream());

                    while (btSocket.isConnected()) {

                        try {
                            int bytesavailable = mmInStream.available();

                            if (bytesavailable > 0) {
                                byte[] curBuf = new byte[bytesavailable];
                                int bytes = mmInStream.read(curBuf);

                                if (bytes != -1 && bytes >= 0) {
                                    mProcessToTerminalIOQueue.write(curBuf, 0, curBuf.length);
                                    mMainThreadHandler.sendEmptyMessage(MSG_NEW_INPUT);
                                }
                            }
                        } catch (IOException e) {
                            Logger.getLogger("app").warning(e.getLocalizedMessage());
                        }
                    }

                    mMainThreadHandler.sendEmptyMessage(MSG_PROCESS_EXITED);
                    //
                } catch (IOException e) {
                    Logger.getLogger("app").info("disconnected!");
                }
            }

        }.start();
    }

    @SuppressLint("HandlerLeak")
    final Handler mMainThreadHandler = new Handler() {
        final byte[] mReceiveBuffer = new byte[4 * 1024];

        @Override
        public void handleMessage(Message msg) {
            if (msg.what == MSG_NEW_INPUT && isRunning()) {
                int bytesRead = mProcessToTerminalIOQueue.read(mReceiveBuffer, false);
                if (bytesRead > 0) {
                    mEmulator.append(mReceiveBuffer, bytesRead);
                    if (new String(mReceiveBuffer).equals("logout")) {

                        mMainThreadHandler.sendEmptyMessage(MSG_PROCESS_EXITED);
                    }
                    notifyScreenUpdate();
                }
            } else if (msg.what == MSG_PROCESS_EXITED) {
                mProcessToTerminalIOQueue.close();
                cleanupResources(0);
                mChangeCallback.onSessionFinished(BluetoothSession.this);

                String exitDescription = "\r\n[Process completed";
                exitDescription += " - press Enter]";

                byte[] bytesToWrite = exitDescription.getBytes(StandardCharsets.UTF_8);
                mEmulator.append(bytesToWrite, bytesToWrite.length);
                notifyScreenUpdate();
            }
        }
    };

    @Override
    public void write(byte[] data, int offset, int count) {
        try {
            mmOutStream.write(data, offset, count);
        } catch (Exception e) {
            //
        }
    }

    @Override
    public void finishIfRunning() {
        if (isRunning()) {
            try {
                disconnect();
            } catch (Exception e) {
                Log.w("termux", "Failed disconnect: " + e.getMessage());
            }
        }
    }

    @Override
    void cleanupResources(int exitStatus) {
        synchronized (this) {
            mShellPid = -1;
            mShellExitStatus = exitStatus;
        }

        // Stop the reader and writer threads, and close the I/O streams
        mProcessToTerminalIOQueue.close();
    }

    private void disconnect() {
        if (btSocket != null) {
            try {
                btSocket.close();
            } catch (Exception e) {
                // do nothing
            }
        }
    }
}
