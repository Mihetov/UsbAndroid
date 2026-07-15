package org.qtproject.example.appUsbAndroid;

import android.app.DownloadManager;
import android.content.Context;
import android.net.Uri;
import android.os.Environment;
import android.util.Log;

public class DeviceDownloader {
    public static void startDownload(Context context, String url, String fileName) {
        try {
            DownloadManager.Request request = new DownloadManager.Request(Uri.parse(url));
            request.setTitle("Device Config");
            // Указываем путь в директорию приложения (не требует лишних прав)
            request.setDestinationInExternalFilesDir(context, "models", fileName);
            request.setNotificationVisibility(DownloadManager.Request.VISIBILITY_VISIBLE_NOTIFY_COMPLETED);

            DownloadManager manager = (DownloadManager) context.getSystemService(Context.DOWNLOAD_SERVICE);
            manager.enqueue(request);
            Log.i("DeviceDownloader", "Запрос в DownloadManager ушел");
        } catch (Exception e) {
            Log.e("DeviceDownloader", "DownloadManager failed: " + e.getMessage());
        }
    }
}