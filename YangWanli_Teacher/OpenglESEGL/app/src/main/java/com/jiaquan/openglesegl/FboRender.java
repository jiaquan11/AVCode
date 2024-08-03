package com.jiaquan.openglesegl;

import android.content.Context;
import android.opengl.GLES20;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

/**
 * 最终的纹理图像渲染类
 * 这个渲染类的操作是把离屏渲染FBO帧缓冲中的纹理完整地渲染到屏幕窗口上
 */
public class FboRender {
    private final float[] mVertexData_ = {
        -1f, -1f,
        1f, -1f,
        -1f, 1f,
        1f, 1f
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
    private int mProgram_;
    private int mVPosition_;
    private int mFPosition_;
    private int mSTexture_;
    private int mVboId_;

    public FboRender(Context context) {
        mContext_ = context;
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
        //vertex_shader:直接绘制纹理，不需要做旋转等操作
        String vertexSource = WLShaderUtil.readRawTxt(mContext_, R.raw.vertex_shader);
        String fragmentSource = WLShaderUtil.readRawTxt(mContext_, R.raw.fragment_shader);
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
        }
    }

    public void onChange(int width, int height) {
        GLES20.glViewport(0, 0, width, height);
    }

    public void onDraw(int textureId) {
        GLES20.glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);

        GLES20.glUseProgram(mProgram_);

        //绑定使用VBO
        GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, mVboId_);
        GLES20.glEnableVertexAttribArray(mVPosition_);
        GLES20.glVertexAttribPointer(mVPosition_, 2, GLES20.GL_FLOAT, false, 8, 0);
        GLES20.glEnableVertexAttribArray(mFPosition_);
        GLES20.glVertexAttribPointer(mFPosition_, 2, GLES20.GL_FLOAT, false, 8, mVertexData_.length * 4);

        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textureId);
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glUniform1i(mSTexture_, 0);

        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
        //解绑纹理id
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 0);
        //解绑VBO
        GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, 0);
    }
}
