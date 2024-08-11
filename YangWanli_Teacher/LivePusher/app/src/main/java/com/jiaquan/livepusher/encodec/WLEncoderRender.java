package com.jiaquan.livepusher.encodec;

import android.content.Context;
import android.graphics.Bitmap;
import android.opengl.GLES20;
import android.opengl.Matrix;

import com.jiaquan.livepusher.R;
import com.jiaquan.livepusher.egl.WLEGLSurfaceView;
import com.jiaquan.livepusher.egl.WLImageUtil;
import com.jiaquan.livepusher.egl.WLShaderUtil;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

public class WLEncoderRender implements WLEGLSurfaceView.WLGLRender {
    private final float[] mVertexData_ = {
            -1f, -1f,
            1f, -1f,
            -1f, 1f,
            1f, 1f,

            0f, 0f,
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
    private FloatBuffer mVertexBuffer_ = null;
    private FloatBuffer mFragmentBuffer_ = null;
    private Context mContext_ = null;
    private int mProgram_ = -1;
    private int mVPosition_;
    private int mFPosition_;
    private int mSTexture_;
    private int mTextureId_ = -1;
    private int mVboId_ = -1;
    private float[] mMatrixArray_ = new float[16];
    private int mUmatrix_;
    private int mSurfaceWidth_ = 0;
    private int mSurfaceHeight_ = 0;
    private int mImageWidth_ = 0;
    private int mImageHeight_ = 0;
    private Bitmap mBitmap_ = null;
    private int mBitmapTextureId_ = 1;
    public WLEncoderRender(Context context) {
        mContext_ = context;
        /**
         * 背景颜色为:#00000000,表示完全透明的黑色，使能opengl的透明功能，则会变成完全透明，否则是默认黑色
         * #000000表示纯黑色   #FFFFFF表示纯白色
         * rgba(255, 255, 255, 0)则表示完全透明的白色
         * rgba(0, 0, 0,1 )则表示完全不透明度的黑色
         */
        mBitmap_ = WLImageUtil.createTextImage("视频直播和推流:jiaquan", 50, "#ff0000", "#00000000", 0);
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
        Matrix.setIdentityM(mMatrixArray_, 0);//初始化单位矩阵
    }

    @Override
    public void onSurfaceCreated() {
        /**
         * 开启透明功能，开启此功能，设置有透明就会透明，否则默认是黑色
         * 针对的是文字图片叠加的场景
         * 开启混合功能
         */
        GLES20.glEnable(GLES20.GL_BLEND);
        GLES20.glBlendFunc(GLES20.GL_SRC_ALPHA, GLES20.GL_ONE_MINUS_SRC_ALPHA);
        String vertexSource = WLShaderUtil.readRawTxt(mContext_, R.raw.vertex_shader_m);
        String fragmentSource = WLShaderUtil.readRawTxt(mContext_, R.raw.fragment_shader_screen);
        mProgram_ = WLShaderUtil.createProgram(vertexSource, fragmentSource);
        if (mProgram_ > 0) {
            mVPosition_ = GLES20.glGetAttribLocation(mProgram_, "v_Position");
            mFPosition_ = GLES20.glGetAttribLocation(mProgram_, "f_Position");
            mUmatrix_ = GLES20.glGetUniformLocation(mProgram_, "u_Matrix");
            mSTexture_ = GLES20.glGetUniformLocation(mProgram_, "mSTexture_");
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
            mBitmapTextureId_ = WLImageUtil.loadBitmapTexture(mBitmap_);
        }
    }

    @Override
    public void onSurfaceChanged(int width, int height) {
        GLES20.glViewport(0, 0, width, height);
        mSurfaceWidth_ = width;
        mSurfaceHeight_ = height;
    }

    @Override
    public void onDrawFrame() {
        GLES20.glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
        GLES20.glUseProgram(mProgram_);

        GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, mVboId_);
        GLES20.glEnableVertexAttribArray(mVPosition_);
        GLES20.glVertexAttribPointer(mVPosition_, 2, GLES20.GL_FLOAT, false, 8, 0);
        GLES20.glEnableVertexAttribArray(mFPosition_);
        GLES20.glVertexAttribPointer(mFPosition_, 2, GLES20.GL_FLOAT, false, 8, mVertexData_.length * 4);
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextureId_);
        GLES20.glUniform1i(mSTexture_, 0);
        _setMatrix(mSurfaceWidth_, mSurfaceHeight_, mImageWidth_, mImageHeight_);
        GLES20.glUniformMatrix4fv(mUmatrix_, 1, false, mMatrixArray_, 0);
        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
        //mBitmap_ 水印图片渲染
        GLES20.glEnableVertexAttribArray(mVPosition_);
        GLES20.glVertexAttribPointer(mVPosition_, 2, GLES20.GL_FLOAT, false, 8, 32);
        GLES20.glEnableVertexAttribArray(mFPosition_);
        GLES20.glVertexAttribPointer(mFPosition_, 2, GLES20.GL_FLOAT, false, 8, mVertexData_.length * 4);
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mBitmapTextureId_);
        GLES20.glUniform1i(mSTexture_, 0);
        _setMatrix(mSurfaceWidth_, mSurfaceHeight_, mImageWidth_, mImageHeight_);
        GLES20.glUniformMatrix4fv(mUmatrix_, 1, false, mMatrixArray_, 0);
        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 0);
        GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, 0);
    }

    @Override
    public void onSurfaceDestroy() {
        GLES20.glDeleteProgram(mProgram_);
        GLES20.glDeleteBuffers(1, new int[]{mVboId_}, 0);
        GLES20.glDeleteTextures(1, new int[]{mTextureId_}, 0);
        GLES20.glDeleteTextures(1, new int[]{mBitmapTextureId_}, 0);
        mProgram_ = -1;
        mVboId_ = -1;
        mBitmapTextureId_ = -1;
        mTextureId_ = -1;
    }

    public void setTexture(int textureId, int imageWidth, int imageHeight) {
        mTextureId_ = textureId;
        mImageWidth_ = imageWidth;
        mImageHeight_ = imageHeight;
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
