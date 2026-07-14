package org.qtproject.example.usbandroid;

import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;
import android.os.Build;
import android.util.Log;

import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialPort;
import com.hoho.android.usbserial.driver.UsbSerialProber;

import java.io.IOException;
import java.util.List;

public final class UsbSerialBridge {
    private static final String TAG = "MODBUS_USB";
    private static final String ACTION_USB_PERMISSION = "org.qtproject.example.usbandroid.USB_PERMISSION";
    private static final int WRITE_WAIT_MILLIS = 1000;

    // Статически сохраняем подключение, чтобы не открывать его заново при каждом write
    private static UsbDeviceConnection mConnection = null;
    private static UsbSerialPort mPort = null;
    private static String mCurrentDeviceName = "";

    private static BroadcastReceiver mUsbReceiver = null;
    private static boolean mReceiverRegistered = false;

    private UsbSerialBridge() {
    }

    // =========================================================================
    // 1. Умный метод write (Сохраняет оригинальную сигнатуру для совместимости с C++)
    // =========================================================================
    public static synchronized int write(Context context, String deviceName, byte[] payload, int baudRate) {
        // Если порт уже открыт для ЭТОГО устройства — просто пишем данные (работает мгновенно!)
        if (mPort != null && mPort.isOpen() && mCurrentDeviceName.equals(deviceName)) {
            try {
                mPort.write(payload, WRITE_WAIT_MILLIS);
                return payload.length;
            } catch (IOException e) {
                Log.e(TAG, "Write failed on persistent port, trying to reopen: " + e.getMessage());
                close(); // Сбрасываем проблемный порт и пробуем переоткрыть ниже
            }
        }

        // Если порт закрыт или сменилось устройство — инициализируем подключение заново
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
                Log.w(TAG, "Permission missing for device: " + deviceName + ". Requesting...");
                requestPermission(context, manager, device, baudRate);
                return -101; // permission must be requested
            }

            mConnection = manager.openDevice(device);
            if (mConnection == null) {
                return -102; // open failed
            }

            try {
                mPort = driver.getPorts().get(0);
                mPort.open(mConnection);
                mPort.setParameters(baudRate, 8, UsbSerialPort.STOPBITS_1, UsbSerialPort.PARITY_NONE);
                mCurrentDeviceName = deviceName;

                // Пишем данные
                mPort.write(payload, WRITE_WAIT_MILLIS);
                Log.i(TAG, "Persistent connection established and data sent.");
                return payload.length;
            } catch (IOException | RuntimeException e) {
                Log.e(TAG, "Serial driver open/write failed: " + e.getMessage());
                close();
                return -103; // serial driver failed
            }
        }

        return -104; // usb-serial-for-android did not find a driver
    }

    // =========================================================================
    // 2. Метод чтения для вызова из C++ (когда будешь готов принимать данные)
    // =========================================================================
    public static synchronized byte[] read(int maxBytesToRead, int timeoutMs) {
        if (mPort == null || !mPort.isOpen()) {
            return new byte[0]; // Порт не открыт
        }

        byte[] buffer = new byte[maxBytesToRead];
        try {
            int bytesRead = mPort.read(buffer, timeoutMs);
            if (bytesRead <= 0) {
                return new byte[0];
            }
            if (bytesRead < maxBytesToRead) {
                byte[] realData = new byte[bytesRead];
                System.arraycopy(buffer, 0, realData, 0, bytesRead);
                return realData;
            }
            return buffer;
        } catch (IOException e) {
            Log.e(TAG, "Read failed: " + e.getMessage());
            return new byte[0];
        }
    }

    // =========================================================================
    // 3. Метод явного закрытия порта
    // =========================================================================
    public static synchronized void close() {
        try {
            if (mPort != null) {
                if (mPort.isOpen()) {
                    mPort.close();
                }
                mPort = null;
            }
        } catch (IOException ignored) {
        } finally {
            if (mConnection != null) {
                mConnection.close();
                mConnection = null;
            }
            mCurrentDeviceName = "";
            Log.d(TAG, "Connection closed and resources cleared.");
        }
    }

    // =========================================================================
    // Логика запроса прав
    // =========================================================================
    private static synchronized void requestPermission(final Context context, final UsbManager manager, final UsbDevice device, final int baudRate) {
        if (mReceiverRegistered) return;

        mUsbReceiver = new BroadcastReceiver() {
            @Override
            @SuppressWarnings("deprecation")
            public void onReceive(Context ctx, Intent intent) {
                String action = intent.getAction();
                if (ACTION_USB_PERMISSION.equals(action)) {
                    synchronized (this) {
                        UsbDevice usbDevice;
                        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                            usbDevice = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE, UsbDevice.class);
                        } else {
                            usbDevice = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
                        }

                        if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
                            if (usbDevice != null) {
                                Log.i(TAG, "Permission GRANTED for device: " + usbDevice.getDeviceName());
                                // Опционально: пишем пустой массив, чтобы просто открыть порт в фоне
                                write(context, usbDevice.getDeviceName(), new byte[0], baudRate);
                            }
                        } else {
                            Log.e(TAG, "Permission DENIED for device: " + (usbDevice != null ? usbDevice.getDeviceName() : "unknown"));
                        }
                    }
                    unregisterPermissionReceiver(context);
                }
            }
        };

        IntentFilter filter = new IntentFilter(ACTION_USB_PERMISSION);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            context.registerReceiver(mUsbReceiver, filter, Context.RECEIVER_NOT_EXPORTED);
        } else {
            context.registerReceiver(mUsbReceiver, filter);
        }
        mReceiverRegistered = true;

        Intent intent = new Intent(ACTION_USB_PERMISSION);
        intent.setPackage(context.getPackageName());

        int flags = PendingIntent.FLAG_UPDATE_CURRENT;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            flags |= PendingIntent.FLAG_MUTABLE;
        }

        PendingIntent permissionIntent = PendingIntent.getBroadcast(context, 0, intent, flags);
        manager.requestPermission(device, permissionIntent);
    }

    private static synchronized void unregisterPermissionReceiver(Context context) {
        if (!mReceiverRegistered || mUsbReceiver == null) return;
        try {
            context.unregisterReceiver(mUsbReceiver);
            mUsbReceiver = null;
            mReceiverRegistered = false;
            Log.d(TAG, "Permission receiver unregistered.");
        } catch (IllegalArgumentException e) {
            Log.e(TAG, "Receiver was already unregistered", e);
        }
    }
}