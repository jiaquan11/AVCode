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
    private byte[] mPixelsBuffer_ = null;
    private List<Integer> mImgList_ = new ArrayList<>();
    private int mIndex_ = -1;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mWLSurfaceView_ = findViewById(R.id.wlSurfaceview);
        mNativeOpengl_ = new NativeOpengl();
        mWLSurfaceView_.setNativeOpengl(mNativeOpengl_);

        mImgList_.add(R.drawable.mingren);
        mImgList_.add(R.drawable.img_1);
        mImgList_.add(R.drawable.img_2);
        mImgList_.add(R.drawable.img_3);

        mWLSurfaceView_.setOnSurfaceListener(new WlSurfaceView.OnSurfaceListener() {
            @Override
            public void init() {//底层GL环境全部初始化好后，才进行读取图像数据进行渲染
                _readPixels();//默认先读取第一张图片进行渲染
            }
        });
    }

    /**
     * 切换滤镜
     */
    public void changeFilter(View view) {
        if (mNativeOpengl_ != null) {
            mNativeOpengl_.surfaceChangeFilter();
        }
    }

    /**
     * 切换纹理
     */
    public void changeTexture(View view) {
        _readPixels();
    }

    /**
     * 读取图片像素数据
     */
    private void _readPixels() {
        //解码一张bitmap图片，拿到像素数据
        final Bitmap bitmap = BitmapFactory.decodeResource(getResources(), _getImageIds());
        ByteBuffer fcbuffer = ByteBuffer.allocate(bitmap.getHeight() * bitmap.getWidth() * 4);
        bitmap.copyPixelsToBuffer(fcbuffer);
        fcbuffer.flip();
        mPixelsBuffer_ = fcbuffer.array();//转换为字节数组
        mNativeOpengl_.imgData(bitmap.getWidth(), bitmap.getHeight(), mPixelsBuffer_.length, mPixelsBuffer_);
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