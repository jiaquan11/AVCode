package com.jiaquan.livepusher.yuv;

import android.content.Context;
import android.opengl.GLES20;
import android.opengl.Matrix;
import android.util.Log;

import com.jiaquan.livepusher.R;
import com.jiaquan.livepusher.egl.WLEGLSurfaceView;
import com.jiaquan.livepusher.egl.WLShaderUtil;

import java.nio.Buffer;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

public class WLYuvRender implements WLEGLSurfaceView.WLGLRender {
    private final float[] mVertexData_ = {
        1f, 1f,
        -1f, 1f,
        1f, -1f,
        -1f, -1f
    };
    private final float[] mTextureVertexData_ = {
        1f, 1f,//FBO坐标
        0f, 1f,
        1f, 0f,
        0f, 0f
    };
    private Context mContext_;
    private FloatBuffer mVertexBuffer_;
    private FloatBuffer mTextureBuffer_;
    private int mProgram_;
    private int mVPosition_;
    private int mFPosition_;
    private int mSamplerY_;
    private int mSamplerU_;
    private int mSamplerV_;
    private int mUmatrix_;
    private int[] mTextureYuv_;
    private int mVboId_;
    private int mFboId_;
    private int mTextureId_;
    private int mYuvwidth_ = 0;
    private int mYuvHeight_ = 0;
    private Buffer mYData_ = null;
    private Buffer mUData_ = null;
    private Buffer mVData = null;
    private float[] mMatrixArray_ = new float[16];
    private WLYuvFboRender mWlYuvFboRender_ = null;
    private int mSurfaceWidth_;
    private int mSurfaceHeight_;
    public WLYuvRender(Context context) {
        mContext_ = context;
        mVertexBuffer_ = ByteBuffer.allocateDirect(mVertexData_.length * 4)
                                   .order(ByteOrder.nativeOrder())
                                   .asFloatBuffer()
                                   .put(mVertexData_);
        mVertexBuffer_.position(0);
        mTextureBuffer_ = ByteBuffer.allocateDirect(mTextureVertexData_.length * 4)
                                    .order(ByteOrder.nativeOrder())
                                    .asFloatBuffer()
                                    .put(mTextureVertexData_);
        mTextureBuffer_.position(0);
        Matrix.setIdentityM(mMatrixArray_, 0);//初始化单位矩阵
        mWlYuvFboRender_ = new WLYuvFboRender(context);
    }

    @Override
    public void onSurfaceCreated() {
        String vertexShader = WLShaderUtil.readRawTxt(mContext_, R.raw.vertex_shader_yuv);
        String fragmentShader = WLShaderUtil.readRawTxt(mContext_, R.raw.fragment_shader_yuv);
        mProgram_ = WLShaderUtil.createProgram(vertexShader, fragmentShader);
        mVPosition_ = GLES20.glGetAttribLocation(mProgram_, "v_Position");
        mFPosition_ = GLES20.glGetAttribLocation(mProgram_, "f_Position");
        mUmatrix_ = GLES20.glGetUniformLocation(mProgram_, "u_Matrix");
        mSamplerY_ = GLES20.glGetUniformLocation(mProgram_, "sampler_y");
        mSamplerU_ = GLES20.glGetUniformLocation(mProgram_, "sampler_u");
        mSamplerV_ = GLES20.glGetUniformLocation(mProgram_, "sampler_v");

        //1.创建VBO
        int[] vbos = new int[1];
        GLES20.glGenBuffers(1, vbos, 0);
        mVboId_ = vbos[0];
        //2.绑定VBO
        GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, mVboId_);
        //3.分配VBO需要的缓存大小
        GLES20.glBufferData(GLES20.GL_ARRAY_BUFFER, mVertexData_.length * 4 + mTextureVertexData_.length * 4, null, GLES20.GL_STATIC_DRAW);
        //4.为VBO设置顶点数据的值
        GLES20.glBufferSubData(GLES20.GL_ARRAY_BUFFER, 0, mVertexData_.length * 4, mVertexBuffer_);
        GLES20.glBufferSubData(GLES20.GL_ARRAY_BUFFER, mVertexData_.length * 4, mTextureVertexData_.length * 4, mTextureBuffer_);
        //5.解绑VBO
        GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, 0);

        mTextureYuv_ = new int[3];
        GLES20.glGenTextures(3, mTextureYuv_, 0);
        for (int i = 0; i < 3; i++) {
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextureYuv_[i]);
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 0);
        }

        /**
         * FBO Render初始化
         * 这个实例对象是用于将FBO中已处理好的纹理图像绘制到屏幕上
         */
        mWlYuvFboRender_.onCreate();
    }

    @Override
    public void onSurfaceChanged(int width, int height) {
        GLES20.glViewport(0, 0, width, height);
        mSurfaceWidth_ = width;
        mSurfaceHeight_ = height;
        _initFBO(width, height);
        mWlYuvFboRender_.onChange(width, height);
    }

    @Override
    public void onDrawFrame() {
        GLES20.glClearColor(1f, 0f, 0f, 1f);
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
        if ((mYuvwidth_ > 0) && (mYuvHeight_ > 0) && (mYData_ != null) && (mUData_ != null) && (mVData != null)) {
            GLES20.glUseProgram(mProgram_);
            GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, mFboId_);
            //绑定使用VB0
            GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, mVboId_);
            GLES20.glEnableVertexAttribArray(mVPosition_);
            GLES20.glVertexAttribPointer(mVPosition_, 2, GLES20.GL_FLOAT, false, 8, 0);
            GLES20.glEnableVertexAttribArray(mFPosition_);
            GLES20.glVertexAttribPointer(mFPosition_, 2, GLES20.GL_FLOAT, false, 8, mVertexData_.length * 4);
            _setMatrix(mSurfaceWidth_, mSurfaceHeight_, mYuvwidth_, mYuvHeight_);
            GLES20.glUniformMatrix4fv(mUmatrix_, 1, false, mMatrixArray_, 0);

            GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextureYuv_[0]);
            GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_LUMINANCE, mYuvwidth_, mYuvHeight_, 0, GLES20.GL_LUMINANCE, GLES20.GL_UNSIGNED_BYTE, mYData_);
            GLES20.glUniform1i(mSamplerY_, 0);

            GLES20.glActiveTexture(GLES20.GL_TEXTURE1);
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextureYuv_[1]);
            GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_LUMINANCE, mYuvwidth_ / 2, mYuvHeight_ / 2, 0, GLES20.GL_LUMINANCE, GLES20.GL_UNSIGNED_BYTE, mUData_);
            GLES20.glUniform1i(mSamplerU_, 1);

            GLES20.glActiveTexture(GLES20.GL_TEXTURE2);
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextureYuv_[2]);
            GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_LUMINANCE, mYuvwidth_ / 2, mYuvHeight_ / 2, 0, GLES20.GL_LUMINANCE, GLES20.GL_UNSIGNED_BYTE, mVData);
            GLES20.glUniform1i(mSamplerV_, 2);
            mYData_.clear();
            mUData_.clear();
            mVData.clear();
            mYData_ = null;
            mUData_ = null;
            mVData = null;
        }
        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 0);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 1);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 2);
        GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, 0);
        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);
        mWlYuvFboRender_.onDraw(mTextureId_);
    }

    public void setYuvData(int yuvWidth, int yuvHeight, byte[] ydata, byte[] udata, byte[] vdata) {
        mYuvwidth_ = yuvWidth;
        mYuvHeight_ = yuvHeight;
        mYData_ = ByteBuffer.wrap(ydata);
        mUData_ = ByteBuffer.wrap(udata);
        mVData = ByteBuffer.wrap(vdata);
    }

    private void _initFBO(int surfaceWidth, int surfaceHeight) {
        int[] fbos = new int[1];
        GLES20.glGenBuffers(1, fbos, 0);
        mFboId_ = fbos[0];
        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, mFboId_);
        int[] textureIds = new int[1];
        GLES20.glGenTextures(1, textureIds, 0);
        mTextureId_ = textureIds[0];
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextureId_);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_REPEAT);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_REPEAT);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
        //设置FBO分配内存大小
        GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_RGBA, surfaceWidth, surfaceHeight, 0, GLES20.GL_RGBA, GLES20.GL_UNSIGNED_BYTE, null);
        GLES20.glFramebufferTexture2D(GLES20.GL_FRAMEBUFFER, GLES20.GL_COLOR_ATTACHMENT0, GLES20.GL_TEXTURE_2D, mTextureId_, 0);
        if (GLES20.glCheckFramebufferStatus(GLES20.GL_FRAMEBUFFER) != GLES20.GL_FRAMEBUFFER_COMPLETE) {
            Log.e("WLYuvRender", "FBO wrong");
        }
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 0);
        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);
    }

    public void _setMatrix(int surfaceWidth, int surfaceHeight, int imageWidth, int imageHeight) {
        float screen_r = surfaceWidth * 1.0f / surfaceHeight;
        float picture_r = imageWidth * 1.0f / imageHeight;
        if (screen_r > picture_r) {//屏幕宽高比大于图片宽高比
            float r = surfaceWidth / (1.0f * surfaceHeight / imageHeight * imageWidth);
            Matrix.orthoM(mMatrixArray_, 0, -r, r, -1f, 1f, -1f, 1f);
        } else {//屏幕宽高比小于图片宽高比
            float r = surfaceHeight / (1.0f * surfaceWidth / imageWidth * imageHeight);
            Matrix.orthoM(mMatrixArray_, 0, -1, 1, -r, r, -1f, 1f);
        }

        /**
         * 旋转矩阵
         * FBO中的纹理坐标系是左下角为原点，而屏幕坐标系是左上角为原点
         * 在FBO一开始设置的纹理坐标系按照左下角为原点，那么这里就不需要旋转，直接绘制即可，
         * 都保持各自正确的坐标体系，就不需要额外操作
         */
//        Matrix.rotateM(mMatrixArray_, 0, 180, 1, 0, 0);//沿着X轴旋转180度，即上下翻转
//      Matrix.rotateM(matrix, 0, 180, 0, 0, 1);//沿着Z轴旋转180度，即逆时针旋转180度(效果不仅仅是上下翻转，还有左右翻转)
    }
}
