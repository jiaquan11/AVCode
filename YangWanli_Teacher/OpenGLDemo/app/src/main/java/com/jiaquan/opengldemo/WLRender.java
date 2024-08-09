package com.jiaquan.opengldemo;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.opengl.GLUtils;
import android.util.Log;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * GL渲染器
 */
public class WLRender implements GLSurfaceView.Renderer {
    private Context mContext_ = null;
    private final float[] mVertexData_ = {
        -1f, -1f,
        1f, -1f,
        -1f, 1f,
        1f, 1f
    };

    private final float[] mTextureData_ = {
        1f, 0f,
        0f, 0f,
        1f, 1f,
        0f, 1f
    };
    private FloatBuffer mVertexBuffer_;
    private FloatBuffer mTextureBuffer_;
    private int mProgram_;
    private int mAvPosition_;
    private int mAfPosition_;
    private int mSTexture_;
    private int mTextureId_;
    private Bitmap mBitmap_ = null;
    public WLRender(Context ctx) {
        mContext_ = ctx;
        mVertexBuffer_ = ByteBuffer.allocateDirect(mVertexData_.length * 4)
                                   .order(ByteOrder.nativeOrder())
                                   .asFloatBuffer()
                                   .put(mVertexData_);
        mVertexBuffer_.position(0);

        mTextureBuffer_ = ByteBuffer.allocateDirect(mTextureData_.length * 4)
                                    .order(ByteOrder.nativeOrder())
                                    .asFloatBuffer()
                                    .put(mTextureData_);
        mTextureBuffer_.position(0);
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        Log.i("WLRender", "onSurfaceCreated in");
        String vertexSource = WLShaderUtil.readRawTxt(mContext_, R.raw.vertex_shader);
        String fragmentSource = WLShaderUtil.readRawTxt(mContext_, R.raw.fragment_shader);
        mProgram_ = WLShaderUtil.createProgram(vertexSource, fragmentSource);
        if (mProgram_ > 0) {
            mAvPosition_ = GLES20.glGetAttribLocation(mProgram_, "av_Position");
            mAfPosition_ = GLES20.glGetAttribLocation(mProgram_, "af_Position");
            mSTexture_ = GLES20.glGetUniformLocation(mProgram_, "sTexture");
            int[] textureIds = new int[1];
            GLES20.glGenTextures(1, textureIds, 0);
            if (textureIds[0] == 0) {
                return;
            }
            mTextureId_ = textureIds[0];
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextureId_);
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_REPEAT);
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_REPEAT);
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);

            /**
             * 将图片数据一次性加载到GPU中,即可释放图片资源
             */
            mBitmap_ = BitmapFactory.decodeResource(mContext_.getResources(), R.drawable.mingren);
            if (mBitmap_ == null) {
                Log.e("WLRender", "onSurfaceCreated: mBitmap_ is null");
            }
            // 加载位图数据到纹理
            GLUtils.texImage2D(GLES20.GL_TEXTURE_2D, 0, mBitmap_, 0);
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 0);
            // 释放位图数据
            mBitmap_.recycle();
            mBitmap_ = null;
        }
        Log.i("WLRender", "onSurfaceCreated end");
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        Log.i("WLRender", "onSurfaceChanged in");
        GLES20.glViewport(0, 0, width, height);
        Log.i("WLRender", "onSurfaceChanged end");
    }

    @Override
    public void onDrawFrame(GL10 gl) {
//        Log.i("WLRender", "onDrawFrame in");
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
        GLES20.glClearColor(1.0f, 1.0f, 1.0f, 1.0f);//白色清屏

        GLES20.glUseProgram(mProgram_);

        GLES20.glEnableVertexAttribArray(mAvPosition_);
        GLES20.glVertexAttribPointer(mAvPosition_, 2, GLES20.GL_FLOAT, false, 8, mVertexBuffer_);

        GLES20.glEnableVertexAttribArray(mAfPosition_);
        GLES20.glVertexAttribPointer(mAfPosition_, 2, GLES20.GL_FLOAT, false, 8, mTextureBuffer_);

        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextureId_);
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glUniform1i(mSTexture_, 0);

        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 0);
//        Log.i("WLRender", "onDrawFrame end");
    }
}
