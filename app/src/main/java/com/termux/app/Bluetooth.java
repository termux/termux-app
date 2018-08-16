package com.termux.app;


import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.app.Activity;
import android.content.Intent;
import android.content.Context;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.Set;

import static android.support.v4.app.ActivityCompat.startActivityForResult;

public class Bluetooth {

    private BluetoothAdapter bluetoothAdapter = null;
    private Context mcontext;
    public Bluetooth(Context mcontext) {
        this.mcontext = mcontext;
        bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
    }

    /**
     * @return ArrayList
     * @throws Exception
     */
    public ArrayList<String[]> getPairedDeviceList() throws Exception {
        bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        Activity mactivity = (Activity) mcontext;

        ArrayList<String[]> list = new ArrayList<>();

        if (bluetoothAdapter == null) {
            //Show a message that the device has no bluetooth adapter
            Toast.makeText(mcontext, "Bluetooth Device Not Available", Toast.LENGTH_LONG).show();
        } else if (!bluetoothAdapter.isEnabled()) {
            //Ask to the user turn the bluetooth on
            Intent turnBTon = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivityForResult(mactivity, turnBTon, 1, null);
        }
        else if (bluetoothAdapter.isEnabled()) {

            Set<BluetoothDevice> pairedDevices;
            pairedDevices = bluetoothAdapter.getBondedDevices();

            if (pairedDevices.size() > 0) {
                for (BluetoothDevice bt : pairedDevices) {
                    String[] device = new String[2];
                    device[0] = bt.getName();
                    device[1] = bt.getAddress();
                    list.add(device);
                }
            } else {
                throw new Exception("no bluetooth devices found.");
            }
        }
        return list;
    }

}
