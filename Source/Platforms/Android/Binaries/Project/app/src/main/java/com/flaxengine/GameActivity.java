// Copyright (c) Wojciech Figat. All rights reserved.

package com.flaxengine;

import android.app.NativeActivity;
import android.app.PendingIntent;
import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.content.DialogInterface;
import android.content.pm.ApplicationInfo;
import android.content.res.AssetManager;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Bundle;
import android.os.Build;
import android.os.Vibrator;
import android.view.View;
import android.view.Display;
import android.view.KeyEvent;
import android.util.Log;
import android.util.DisplayMetrics;
import android.net.Uri;
import android.net.NetworkInfo;
import android.net.ConnectivityManager;
import android.media.AudioManager;
import android.provider.Settings;
import android.graphics.Point;
import android.annotation.TargetApi;

import java.util.concurrent.Semaphore;
import java.io.File;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.FileNotFoundException;
import java.net.URL;
import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;

public class GameActivity extends NativeActivity {
    static {
        try {
            // Load native libraries
            System.loadLibrary("FlaxGame");
        } catch (UnsatisfiedLinkError error) {
            Log.e("Flax", error.getMessage());
        }
    }

    static GameActivity _activity;
    String _appPackageName;
    final Semaphore _semaphore = new Semaphore(0, true);

	/** Gets the singleton activity for the game. **/
	public static GameActivity Get() {
		return _activity;
    }

	private class VibrateRunnable implements Runnable {
		private int duration;
		private Vibrator vibrator;

		VibrateRunnable(final int duration, final Vibrator vibrator) {
			this.duration = duration;
			this.vibrator = vibrator;
        }

		public void run () {
			if (duration < 1) {
				vibrator.cancel();
			} else {
				vibrator.vibrate(duration);
			}
		}
    }

    public void vibrate(int duration) {
		Vibrator vibrator = (Vibrator)getSystemService(VIBRATOR_SERVICE);
		if (vibrator != null) {
			_activity.runOnUiThread(new VibrateRunnable(duration, vibrator));
		}
    }

    public void showAlert(final String text, final String caption) {
        final GameActivity activity = this;
        activity.runOnUiThread(new Runnable() {
            public void run() {
                AlertDialog.Builder builder = new AlertDialog.Builder(activity, AlertDialog.THEME_HOLO_DARK);
                builder.setTitle(caption);
                builder.setMessage(text);
                builder.setPositiveButton("Close", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                        _semaphore.release();
                    }
                });
                builder.setCancelable(false);
                AlertDialog dialog = builder.create();
                dialog.show();
            }
        });
        try {
            _semaphore.acquire();
        }
        catch (InterruptedException e) {
        }
    }

	public int getRotation() {
		return getWindowManager().getDefaultDisplay().getRotation();
    }

	@SuppressWarnings("deprecation")
	@TargetApi(Build.VERSION_CODES.JELLY_BEAN_MR1)
	static boolean isAirplaneModeOn(Context context) {
		if (Build.VERSION.SDK_INT < Build.VERSION_CODES.JELLY_BEAN_MR1) {
			return Settings.System.getInt(context.getContentResolver(), Settings.System.AIRPLANE_MODE_ON, 0) != 0;
		}
		return Settings.Global.getInt(context.getContentResolver(), Settings.Global.AIRPLANE_MODE_ON, 0) != 0;
    }

    public int getNetworkConnectionType() {
        try {
            if (isAirplaneModeOn(getApplicationContext())) {
                return 2;
            }
            ConnectivityManager cm = (ConnectivityManager)getSystemService(Context.CONNECTIVITY_SERVICE);
            NetworkInfo activeNetwork = cm.getActiveNetworkInfo();
		    if ((activeNetwork != null && activeNetwork.isAvailable() && activeNetwork.isConnectedOrConnecting())) {
			    switch (activeNetwork.getType()) {
				    case ConnectivityManager.TYPE_WIFI: return 4;
				    case ConnectivityManager.TYPE_BLUETOOTH: return 5;
				    case ConnectivityManager.TYPE_ETHERNET: return 6;
			    }
			    return 3;
		    }
            return 0;
        } catch (Exception e) {
            Log.i("Flax", "Error", e);
            return 1;
        }
    }

    public void openUrl(String url) {
        Uri webpage = Uri.parse(url);
        Intent intent = new Intent(Intent.ACTION_VIEW, webpage);
        if (intent.resolveActivity(getPackageManager()) != null) {
            startActivity(intent);
        }
    }

    static void copyAssetDir(AssetManager am, String path, String outpath) {
        try {
            String[] res = am.list(path);
            for (int i = 0; i < res.length; i++) {
                String fromFile = path + "/" + res[i];
                String toFile = outpath + "/" + res[i];
                InputStream fromStream;
                try {
                    fromStream = am.open(fromFile);
                } catch (FileNotFoundException e) {
                    new File(toFile).mkdirs();
                    copyAssetDir(am, fromFile, toFile);
                    continue;
                }
                copy(fromStream, new FileOutputStream(toFile));
            }
        } catch (Exception e) {
            Log.i("Flax", "Error", e);
        }
    }

    static void copy(InputStream in, OutputStream out) throws IOException {
        byte[] buff = new byte[1024];
        for (int len = in.read(buff); len != -1; len = in.read(buff)) {
            out.write(buff, 0, len);
        }
        in.close();
        out.close();
    }

    static String readFileText(InputStream stream, Charset encoding) throws IOException {
        byte[] buffer = new byte[stream.available()];
        stream.read(buffer);
        stream.close();
        return new String(buffer, encoding);
    }

    void extractDotnetFiles() throws IOException
    {
        String filesDir = getFilesDir().getAbsolutePath();
        AssetManager am = getAssets();
        String[] amRootFiles = am.list("");

        // Skip if extracted has is the same as in the package
        String hashFile = "hash.txt";
        File currentHashFile = new File(filesDir + "/" + hashFile);
        if (currentHashFile.exists()) {
            for (int i = 0; i < amRootFiles.length; i++) {
                if (amRootFiles[i].equals(hashFile)) {
                    String currentHash = readFileText(new FileInputStream(currentHashFile), StandardCharsets.US_ASCII);
                    String hash = readFileText(am.open(hashFile), StandardCharsets.US_ASCII);
                    if (currentHash.equals(hash)) {
                        return;
                    }
                    break;
                }
            }
        }

        // Extract files
        Log.i("Flax", "Extracting Dotnet files");
        new File(filesDir + "/Dotnet").mkdir();
        copyAssetDir(am, "Dotnet", filesDir + "/Dotnet");
        for (int i = 0; i < amRootFiles.length; i++) {
            String fromFile = amRootFiles[i];
            if (!fromFile.endsWith(".dll") && !fromFile.equals(hashFile))
                continue;
            String toFile = filesDir + "/" + amRootFiles[i];
            InputStream fromStream = am.open(fromFile);
            copy(fromStream, new FileOutputStream(toFile));
        }
    }

    void goFullscreen() {
        View decorView = getWindow().getDecorView();
        decorView.setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY |
                View.SYSTEM_UI_FLAG_LAYOUT_STABLE |
                View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION |
                View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN |
                View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
                View.SYSTEM_UI_FLAG_FULLSCREEN);
    }

    @Override
    protected void onCreate(Bundle instance) {
        _activity = this;

        // Extract Dotnet files and other bundled C# libraries from APK (Mono uses unix file access API which doesn't work with AAssetManager API)
        try {
            extractDotnetFiles();
        } catch (Exception e) {
            Log.i("Flax", "Error", e);
        }

        // Set native engine platform info
        String appPackageName = getPackageName();
        String deviceManufacturer = android.os.Build.MANUFACTURER;
        String deviceModel = android.os.Build.MODEL;
        String deviceBuildNumber = android.os.Build.DISPLAY;
        String systemVersion = android.os.Build.VERSION.RELEASE;
        String systemLanguage = java.util.Locale.getDefault().toString();
        Display display = getWindowManager().getDefaultDisplay();
        Point displayRealSize = new Point();
		display.getRealSize(displayRealSize);
        int screenWidth = displayRealSize.x;
        int screenHeight = displayRealSize.y;
        String cacheDir = getCacheDir().toString();
        String executablePath = getApplicationInfo().nativeLibraryDir + "/libFlaxGame.so";
        nativeSetPlatformInfo(appPackageName, deviceManufacturer, deviceModel, deviceBuildNumber, systemVersion, systemLanguage, screenWidth, screenHeight, cacheDir, executablePath);

        super.onCreate(instance);

        // Initialize
        setVolumeControlStream(AudioManager.STREAM_MUSIC);
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);

        if (hasFocus) {
            goFullscreen();
        }
    }

	@Override
	public void onResume() {
        super.onResume();
        
        goFullscreen();
    }

	public native void nativeSetPlatformInfo(String appPackageName, String deviceManufacturer, String deviceModel, String deviceBuildNumber, String systemVersion, String systemLanguage, int screenWidth, int screenHeight, String cacheDir, String executablePath);
}
