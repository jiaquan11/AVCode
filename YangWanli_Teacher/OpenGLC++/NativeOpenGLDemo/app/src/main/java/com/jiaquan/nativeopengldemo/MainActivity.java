package com.jiaquan.nativeopengldemo;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.view.View;

import androidx.appcompat.app.AppCompatActivity;

import com.jiaquan.opengl.NativeOpengl;
import com.jiaquan.opengl.WlSurfaceView;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;

public class MainActivity extends AppCompatActivity {
    private WlSurfaceView mWLSurfaceView_ = null;
    private NativeOpengl mNativeOpengl_ = null;
    private List<Integer> mImgList_ = new ArrayList<>();
    private int mIndex_ = -1;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mWLSurfaceView_ = findViewById(R.id.wlSurfaceview);
        mNativeOpengl_ = new NativeOpengl();
        mWLSurfaceView_.setNativeOpengl(mNativeOpengl_);
        mWLSurfaceView_.setOnSurfaceListener(new NativeOpengl.OnSurfaceListener() {
            @Override
            public void init() {
                _readImagePixels();
            }
        });

        mImgList_.add(R.drawable.mingren);
        mImgList_.add(R.drawable.img_1);
        mImgList_.add(R.drawable.img_2);
        mImgList_.add(R.drawable.img_3);
    }

    /**
     * 切换滤镜
     */
    public void changeFilter(View view) {
        if (mNativeOpengl_ != null) {
            mNativeOpengl_.nativeSurfaceChangeFilter();
        }
    }

    /**
     * 切换纹理
     */
    public void changeTexture(View view) {
        _readImagePixels();
    }

    /**
     * 读取图片像素数据
     */
    private void _readImagePixels() {
        final Bitmap bitmap = BitmapFactory.decodeResource(getResources(), _getImageIds());
        ByteBuffer bmpBuffer = ByteBuffer.allocate(bitmap.getHeight() * bitmap.getWidth() * 4);
        bitmap.copyPixelsToBuffer(bmpBuffer);
        bmpBuffer.flip();
        byte[] pixelsBuffer = bmpBuffer.array();
        mNativeOpengl_.nativeSetImgData(bitmap.getWidth(), bitmap.getHeight(), pixelsBuffer.length, pixelsBuffer);
    }

    /**
     * 获取图片资源id
     */
    private int _getImageIds() {
        mIndex_++;
        if (mIndex_ >= mImgList_.size()) {
            mIndex_ = 0;
        }
        return mImgList_.get(mIndex_);
    }
}