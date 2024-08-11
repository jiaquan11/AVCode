package com.jiaquan.livepusher.imgvideo;

import android.content.Context;
import android.opengl.GLES20;

import com.jiaquan.livepusher.R;
import com.jiaquan.livepusher.egl.WLShaderUtil;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

public class WLImgFboRender {
    private float[] mVertexData_ = {
            -1f, -1f,
            1f, -1f,
            -1f, 1f,
            1f, 1f
    };
    private float[] mFragmentData_ = {
            0f, 1f,
            1f, 1f,
            0f, 0f,
            1f, 0f
    };
    private FloatBuffer mVertexBuffer_;
    private FloatBuffer mFragmentBuffer_;
    private Context mContext_;
    private int mProgram_ = -1;
    private int mVPosition_ = -1;
    private int mFPosition_ = -1;
    private int mSTexture_ = -1;
    private int mVboId_ = -1;
    public WLImgFboRender(Context context) {
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
        mContext_ = context;
    }

    public void onCreate() {
        String vertexSource = WLShaderUtil.readRawTxt(mContext_, R.raw.vertex_shader_screen);
        String fragmentSource = WLShaderUtil.readRawTxt(mContext_, R.raw.fragment_shader_screen);
        mProgram_ = WLShaderUtil.createProgram(vertexSource, fragmentSource);
        mVPosition_ = GLES20.glGetAttribLocation(mProgram_, "v_Position");
        mFPosition_ = GLES20.glGetAttribLocation(mProgram_, "f_Position");
        mSTexture_ = GLES20.glGetUniformLocation(mProgram_, "mSTexture_");
        int[] vbos = new int[1];
        GLES20.glGenBuffers(1, vbos, 0);
        mVboId_ = vbos[0];
        GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, mVboId_);
        GLES20.glBufferData(GLES20.GL_ARRAY_BUFFER, mVertexData_.length * 4 + mFragmentData_.length * 4, null, GLES20.GL_STATIC_DRAW);
        GLES20.glBufferSubData(GLES20.GL_ARRAY_BUFFER, 0, mVertexData_.length * 4, mVertexBuffer_);
        GLES20.glBufferSubData(GLES20.GL_ARRAY_BUFFER, mVertexData_.length * 4, mFragmentData_.length * 4, mFragmentBuffer_);
        GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, 0);
    }

    public void onChange(int width, int height) {
        GLES20.glViewport(0, 0, width, height);
    }

    public void onDraw(int textureId) {
        GLES20.glClearColor(0f, 0f, 1f, 1f);
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
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 0);
        GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, 0);
    }

    public void onDestroy() {
        GLES20.glDeleteProgram(mProgram_);
        GLES20.glDeleteBuffers(1, new int[]{mVboId_}, 0);
        mProgram_ = -1;
        mVboId_ = -1;
    }
}
