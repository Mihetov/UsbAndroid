package org.qtproject.example.usbandroid;

import android.content.Context;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;

import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialPort;
import com.hoho.android.usbserial.driver.UsbSerialProber;

import java.io.IOException;
import java.util.List;

public final class UsbSerialBridge {
    private static final int WRITE_WAIT_MILLIS = 1000;

    private UsbSerialBridge() {
    }

    public static int write(Context context, String deviceName, byte[] payload, int baudRate) {
        UsbManager manager = (UsbManager) context.getSystemService(Context.USB_SERVICE);
        if (manager == null) {
            return -100; // no UsbManager
        }

        List<UsbSerialDriver> drivers = UsbSerialProber.getDefaultProber().findAllDrivers(manager);
        for (UsbSerialDriver driver : drivers) {
            UsbDevice device = driver.getDevice();
            if (!device.getDeviceName().equals(deviceName)) {
                continue;
            }
            if (!manager.hasPermission(device)) {
                return -101; // permission must be requested by Android before opening
            }

            UsbDeviceConnection connection = manager.openDevice(device);
            if (connection == null) {
                return -102; // open failed
            }

            UsbSerialPort port = null;
            try {
                port = driver.getPorts().get(0);
                port.open(connection);
                port.setParameters(baudRate, 8, UsbSerialPort.STOPBITS_1, UsbSerialPort.PARITY_NONE);
                port.write(payload, WRITE_WAIT_MILLIS);
                return payload.length;
            } catch (IOException | RuntimeException e) {
                return -103; // serial driver failed
            } finally {
                if (port != null) {
                    try {
                        port.close();
                    } catch (IOException ignored) {
                    }
                } else {
                    connection.close();
                }
            }
        }

        return -104; // usb-serial-for-android did not find a driver for this device
    }
}
