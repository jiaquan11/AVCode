package com.jiaquan.livepusher.camera;

import android.content.Context;
import android.graphics.ImageFormat;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.util.Log;

import com.jiaquan.livepusher.util.DisplayUtil;

import java.io.IOException;
import java.util.List;

public class WLCamera {
    private final static String TAG = WLCamera.class.getSimpleName();

    private SurfaceTexture surfaceTexture = null;
    private Camera camera = null;

    private int screenWidth = 0;
    private int screenHeight = 0;

    public WLCamera(Context context) {
        //手机屏幕的宽高
        this.screenWidth = DisplayUtil.getScreenWidth(context);
        this.screenHeight = DisplayUtil.getScreenHeight(context);
        Log.i(TAG, "WLCamera screen width: " + screenWidth + " height: " + screenHeight);
    }

    public void initCamera(SurfaceTexture surfaceTexture, int cameraId) {
        this.surfaceTexture = surfaceTexture;

        setCameraParam(cameraId);
    }

    public void stopPreview() {
        if (camera != null) {
            camera.stopPreview();
            camera.release();
            camera = null;
        }
    }

    public void changeCamera(int cameraId) {
        if (camera != null) {
            stopPreview();
        }

        setCameraParam(cameraId);
    }

    private void setCameraParam(int cameraId) {
        Log.i(TAG, "setCameraParam cameraId:" + cameraId);
        try {
            camera = Camera.open(cameraId);
            camera.setPreviewTexture(surfaceTexture);//摄像头预览需要一个surfaceTexture纹理传递数据

            Camera.Parameters parameters = camera.getParameters();
            parameters.setFlashMode("off");
            parameters.setPreviewFormat(ImageFormat.NV21);

            Camera.Size size = getFitSize(parameters.getSupportedPictureSizes());
            parameters.setPictureSize(size.width, size.height);//设置摄像头拍摄的宽高
            Log.i(TAG, "setPictureSize width: " + size.width + " height: " + size.height);
            size = getFitSize(parameters.getSupportedPreviewSizes());//设置摄像头预览的宽高
            parameters.setPreviewSize(size.width, size.height);
            Log.i(TAG, "setPreviewSize width: " + size.width + " height: " + size.height);

            camera.setParameters(parameters);
            camera.startPreview();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private Camera.Size getFitSize(List<Camera.Size> sizes) {
        if (screenWidth < screenHeight) {
            int t = screenHeight;
            screenHeight = screenWidth;
            screenWidth = t;
        }

        for (Camera.Size size : sizes) {//摄像头的可支持尺寸都是(宽>高)的值
            Log.i(TAG, "size.width: " + size.width + ", size.height: " + size.height);
            if ((1.0f * size.width / size.height) == (1.0f * screenWidth / screenHeight)) {//等比例
                return size;
            }
        }

        //没有匹配到等比例，就返回第一组宽高尺寸数据
        Log.i(TAG, "getFitSize sizes.get(0) width: " + sizes.get(0).width + " height: " + sizes.get(0).height);
        return sizes.get(0);
    }
}
