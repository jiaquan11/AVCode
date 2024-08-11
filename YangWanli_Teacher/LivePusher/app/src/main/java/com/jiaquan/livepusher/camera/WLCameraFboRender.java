package com.jiaquan.livepusher.camera;

import android.content.Context;
import android.graphics.Bitmap;
import android.opengl.GLES20;

import com.jiaquan.livepusher.R;
import com.jiaquan.livepusher.egl.WLImageUtil;
import com.jiaquan.livepusher.egl.WLShaderUtil;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

public class WLCameraFboRender {
    private final float[] mVertexData_ = {
            -1f, -1f,
            1f, -1f,
            -1f, 1f,
            1f, 1f,

            0f, 0f,//文字图片的坐标
            0f, 0f,
            0f, 0f,
            0f, 0f
    };

    private final float[] mFragmentData_ = {
            0f, 1f,
            1f, 1f,
            0f, 0f,
            1f, 0f
    };
    private Context mContext_ = null;
    private FloatBuffer mVertexBuffer_ = null;
    private FloatBuffer mFragmentBuffer_ = null;
    private int mProgram_ = -1;
    private int mVPosition_;
    private int mFPosition_;
    private int mSTexture_;
    private int mVboId_ = -1;
    private Bitmap mBitmap_ = null;
    private int mBitmapTextureId_ = -1;

    public WLCameraFboRender(Context context) {
        mContext_ = context;
        mBitmap_ = WLImageUtil.createTextImage("视频直播和推流:jiaquan", 50, "#ff0000", "#00000000", 0);
        /**
         * 这里对文字图片做等比例缩放，使其在窗口中显示的时候不变形
         * 水印图片会绘制到窗口，所以会随着窗口变化而变化
         * 1.计算图片的宽高比
         * 2.计算图片的新宽度。这里的高度强制为0.1，然后通过图片的等比例宽高比计算得到图片新的宽度
         */
        float r = 1.0f * mBitmap_.getWidth() / mBitmap_.getHeight();
        float w = r * 0.1f;
        mVertexData_[8] = 0.8f - w;//左下角坐标
        mVertexData_[9] = -0.8f;

        mVertexData_[10] = 0.8f;//右下角坐标
        mVertexData_[11] = -0.8f;

        mVertexData_[12] = 0.8f - w;//左上角
        mVertexData_[13] = -0.7f;

        mVertexData_[14] = 0.8f;//右上角
        mVertexData_[15] = -0.7f;
        mVertexBuffer_ = ByteBuffer.allocateDirect(mVertexData_.length * 4)
                                   .order(ByteOrder.nativeOrder())
                                   .asFloatBuffer()
                                   .put(mVertexData_);
        mVertexBuffer_.position(0);
        mFragmentBuffer_ = ByteBuffer.allocateDirect(mFragmentData_.length * 4)
                                     .order(ByteOrder.nativeOrder())
                                     .asFloatBuffer()
                                     .put(mFragmentData_);
        mFragmentBuffer_.position(0);
    }

    public void onCreate() {
        /**
         * 开启透明模式和混合模式
         * 主要是为了文字图片叠加到预览画面的透明效果，如果不开启，文字图片会有黑色背景
         */
        GLES20.glEnable(GLES20.GL_BLEND);
        GLES20.glBlendFunc(GLES20.GL_SRC_ALPHA, GLES20.GL_ONE_MINUS_SRC_ALPHA);

        String vertexSource = WLShaderUtil.readRawTxt(mContext_, R.raw.vertex_shader_screen);
        String fragmentSource = WLShaderUtil.readRawTxt(mContext_, R.raw.fragment_shader_screen);
        mProgram_ = WLShaderUtil.createProgram(vertexSource, fragmentSource);
        if (mProgram_ > 0) {
            mVPosition_ = GLES20.glGetAttribLocation(mProgram_, "v_Position");
            mFPosition_ = GLES20.glGetAttribLocation(mProgram_, "f_Position");
            mSTexture_ = GLES20.glGetUniformLocation(mProgram_, "sTexture");
            //1.创建VBO
            int[] vbos = new int[1];
            GLES20.glGenBuffers(1, vbos, 0);
            mVboId_ = vbos[0];
            //2.绑定VBO
            GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, mVboId_);
            //3.分配VBO需要的缓存大小
            GLES20.glBufferData(GLES20.GL_ARRAY_BUFFER, mVertexData_.length * 4 + mFragmentData_.length * 4, null, GLES20.GL_STATIC_DRAW);
            //4.为VBO设置顶点数据的值
            GLES20.glBufferSubData(GLES20.GL_ARRAY_BUFFER, 0, mVertexData_.length * 4, mVertexBuffer_);
            GLES20.glBufferSubData(GLES20.GL_ARRAY_BUFFER, mVertexData_.length * 4, mFragmentData_.length * 4, mFragmentBuffer_);
            //5.解绑VBO
            GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, 0);
            mBitmapTextureId_ = WLImageUtil.loadBitmapTexture(mBitmap_);//加载文字图片纹理
        }
    }

    public void onChange(int width, int height) {
        GLES20.glViewport(0, 0, width, height);
    }

    public void onDraw(int textureId) {
        GLES20.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);//设置清屏颜色为黑色
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
        GLES20.glUseProgram(mProgram_);

        GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, mVboId_);
        GLES20.glEnableVertexAttribArray(mVPosition_);
        GLES20.glVertexAttribPointer(mVPosition_, 2, GLES20.GL_FLOAT, false, 8, 0);
        GLES20.glEnableVertexAttribArray(mFPosition_);
        GLES20.glVertexAttribPointer(mFPosition_, 2, GLES20.GL_FLOAT, false, 8, mVertexData_.length * 4);
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textureId);
        GLES20.glUniform1i(mSTexture_, 0);
        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);

        GLES20.glEnableVertexAttribArray(mVPosition_);
        GLES20.glVertexAttribPointer(mVPosition_, 2, GLES20.GL_FLOAT, false, 8, 32);
        GLES20.glEnableVertexAttribArray(mFPosition_);
        GLES20.glVertexAttribPointer(mFPosition_, 2, GLES20.GL_FLOAT, false, 8, mVertexData_.length * 4);
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mBitmapTextureId_);
        GLES20.glUniform1i(mSTexture_, 0);
        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 0);
        GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, 0);
    }

    public void onDestory() {
        GLES20.glDeleteProgram(mProgram_);
        GLES20.glDeleteBuffers(1, new int[]{mVboId_}, 0);
        GLES20.glDeleteTextures(1, new int[]{mBitmapTextureId_}, 0);
        mProgram_ = -1;
        mVboId_ = -1;
        mBitmapTextureId_ = -1;
    }
}
