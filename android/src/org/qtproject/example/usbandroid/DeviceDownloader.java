package org.qtproject.example.appUsbAndroid;

import android.util.Log;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;

public class DeviceDownloader {
    private static final String TAG = "DeviceDownloader";

    // Этот метод будет вызываться из C++
    public static boolean downloadAndSave(String fileUrl, String savePath) {
        try {
            URL url = new URL(fileUrl);
            HttpURLConnection conn = (HttpURLConnection) url.openConnection();
            conn.setRequestMethod("GET");
            conn.setConnectTimeout(10000);
            conn.setReadTimeout(10000);
            conn.connect();

            int responseCode = conn.getResponseCode();
            if (responseCode != HttpURLConnection.HTTP_OK) {
                Log.e(TAG, "Server returned HTTP error code: " + responseCode);
                return false;
            }

            // Читаем ответ
            BufferedReader reader = new BufferedReader(new InputStreamReader(conn.getInputStream()));
            StringBuilder stringBuilder = new StringBuilder();
            String line;
            while ((line = reader.readLine()) != null) {
                stringBuilder.append(line).append("\n");
            }
            reader.close();

            // Сохраняем в файл по указанному пути
            File file = new File(savePath);
            FileOutputStream fos = new FileOutputStream(file);
            fos.write(stringBuilder.toString().getBytes("UTF-8"));
            fos.close();

            Log.i(TAG, "Successfully downloaded and saved to: " + savePath);
            return true;
        } catch (Exception e) {
            Log.e(TAG, "Download failed: " + e.getMessage());
            e.printStackTrace();
            return false;
        }
    }
}