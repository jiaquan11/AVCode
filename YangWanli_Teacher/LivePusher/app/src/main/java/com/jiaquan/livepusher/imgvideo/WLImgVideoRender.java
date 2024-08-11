package com.jiaquan.livepusher.imgvideo;

import android.content.Context;
import android.opengl.GLES20;
import android.opengl.Matrix;
import android.util.Log;

import com.jiaquan.livepusher.R;
import com.jiaquan.livepusher.egl.TextureInfo;
import com.jiaquan.livepusher.egl.WLEGLSurfaceView;
import com.jiaquan.livepusher.egl.WLImageUtil;
import com.jiaquan.livepusher.egl.WLShaderUtil;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

public class WLImgVideoRender implements WLEGLSurfaceView.WLGLRender {
    private final float[] mVertexData_ = {
            -1f, -1f,
            1f, -1f,
            -1f, 1f,
            1f, 1f,
    };
    private final float[] mFragmentData_ = {
            0f, 0f,
            1f, 0f,
            0f, 1f,
            1f, 1f
    };
    private FloatBuffer mVertexBuffer_;
    private FloatBuffer mFragmentBuffer_;
    private Context mContext_ = null;
    private int mProgram_ = -1;
    private int mVPosition_ = -1;
    private int mFPosition_ = -1;
    private int mTextureid_ = -1;
    private int mSTexture_ = -1;
    private int mVboId_ = -1;
    private int mFboId_ = -1;
    private float[] mMatrixArray_ = new float[16];
    private int mUmatrix_;
    private WLImgFboRender mWlImgFboRender_ = null;
    private int mImageId_ = 0;
    private int mSurfaceWidth_ = 0;
    private int mSurfaceHeight_ = 0;

    private OnRenderCreateListener mOnRenderCreateListener_;
    public void setOnRenderCreateListener(OnRenderCreateListener onRenderCreateListener) {
        mOnRenderCreateListener_ = onRenderCreateListener;
    }
    public interface OnRenderCreateListener {
        void onCreate(int textureId, int surfaceWidth, int surfaceHeight);
    }

    private OnEncodeListener mOnEncodeListener_;
    public void setOnEncodeListener(OnEncodeListener onRenderListener) {
        mOnEncodeListener_ = onRenderListener;
    }
    public interface OnEncodeListener {
        void onEncode();
    }
    public WLImgVideoRender(Context context) {
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
        Matrix.setIdentityM(mMatrixArray_, 0);//初始化单位矩阵
        mWlImgFboRender_ = new WLImgFboRender(context);
    }

    @Override
    public void onSurfaceCreated() {
        String vertexSource = WLShaderUtil.readRawTxt(mContext_, R.raw.vertex_shader_m);
        String fragmentSource = WLShaderUtil.readRawTxt(mContext_, R.raw.fragment_shader_screen);
        mProgram_ = WLShaderUtil.createProgram(vertexSource, fragmentSource);
        mVPosition_ = GLES20.glGetAttribLocation(mProgram_, "v_Position");
        mFPosition_ = GLES20.glGetAttribLocation(mProgram_, "f_Position");
        mUmatrix_ = GLES20.glGetUniformLocation(mProgram_, "u_Matrix");
        mSTexture_ = GLES20.glGetUniformLocation(mProgram_, "sTexture");
        int[] vbos = new int[1];
        GLES20.glGenBuffers(1, vbos, 0);
        mVboId_ = vbos[0];
        GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, mVboId_);
        GLES20.glBufferData(GLES20.GL_ARRAY_BUFFER, mVertexData_.length * 4 + mFragmentData_.length * 4, null, GLES20.GL_STATIC_DRAW);
        GLES20.glBufferSubData(GLES20.GL_ARRAY_BUFFER, 0, mVertexData_.length * 4, mVertexBuffer_);
        GLES20.glBufferSubData(GLES20.GL_ARRAY_BUFFER, mVertexData_.length * 4, mFragmentData_.length * 4, mFragmentBuffer_);
        GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, 0);
        mWlImgFboRender_.onCreate();
    }

    @Override
    public void onSurfaceChanged(int width, int height) {
        Log.i("LivePusherPlayer", "onSurfaceChanged in, width: " + width + " height: " + height);
        GLES20.glViewport(0, 0, width, height);
        if (mSurfaceWidth_ != width || mSurfaceHeight_ != height) {
            _recreateFBO(width, height);
        }
        mSurfaceWidth_ = width;
        mSurfaceHeight_ = height;
        mWlImgFboRender_.onChange(width, height);
        if (mOnRenderCreateListener_ != null) {
            mOnRenderCreateListener_.onCreate(mTextureid_, mSurfaceWidth_, mSurfaceHeight_);
        }
    }

    @Override
    public void onDrawFrame() {
        TextureInfo bmpTextureInfo = WLImageUtil.loadBitmapTexture(mContext_, mImageId_);
        GLES20.glUseProgram(mProgram_);
        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, mFboId_);
        /**
         * 要想在FBO清屏的效果生效，需要先绑定FBO，然后再清屏
         */
        GLES20.glClearColor(1f, 0f, 0f, 1f);
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
        GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, mVboId_);
        GLES20.glEnableVertexAttribArray(mVPosition_);
        GLES20.glVertexAttribPointer(mVPosition_, 2, GLES20.GL_FLOAT, false, 8, 0);
        GLES20.glEnableVertexAttribArray(mFPosition_);
        GLES20.glVertexAttribPointer(mFPosition_, 2, GLES20.GL_FLOAT, false, 8, mVertexData_.length * 4);
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, bmpTextureInfo.mTextureId);
        GLES20.glUniform1i(mSTexture_, 0);
        _setMatrix(mSurfaceWidth_, mSurfaceHeight_, bmpTextureInfo.mWidth, bmpTextureInfo.mHeight);
        GLES20.glUniformMatrix4fv(mUmatrix_, 1, false, mMatrixArray_, 0);
        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 0);
        GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, 0);
        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);

        mWlImgFboRender_.onDraw(mTextureid_);

        /**
         * 通知硬解编码器绘制编码
         */
        if (mOnEncodeListener_ != null) {
            mOnEncodeListener_.onEncode();
        }
        int[] ids = new int[]{bmpTextureInfo.mTextureId};
        GLES20.glDeleteTextures(1, ids, 0);
        bmpTextureInfo.mTextureId = 0;
    }

    @Override
    public void onSurfaceDestroy() {
        GLES20.glDeleteProgram(mProgram_);
        GLES20.glDeleteBuffers(1, new int[]{mVboId_}, 0);
        GLES20.glDeleteTextures(1, new int[]{mTextureid_}, 0);
        GLES20.glDeleteFramebuffers(1, new int[]{mFboId_}, 0);
        mProgram_ = 0;
        mVboId_ = 0;
        mTextureid_ = 0;
        mFboId_ = 0;
        mWlImgFboRender_.onDestroy();
        mWlImgFboRender_ = null;
    }

    public void setCurrentImgageId(int imageId) {
        mImageId_ = imageId;
    }

    private void _recreateFBO(int width, int height) {
        Log.i("LivePusherPlayer", "recreateFBO in");
        _releaseOldFBO();
        _initFBO();
        _mallocFBOBuffer(width, height);
        Log.i("LivePusherPlayer", "recreateFBO end");
    }

    private void _initFBO() {
        Log.i("LivePusherPlayer", "initFBO in");
        int[] fbos = new int[1];
        GLES20.glGenBuffers(1, fbos, 0);
        mFboId_ = fbos[0];
        int[] textureIds = new int[1];
        GLES20.glGenTextures(1, textureIds, 0);
        mTextureid_ = textureIds[0];
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextureid_);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_REPEAT);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_REPEAT);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 0);
        Log.i("LivePusherPlayer", "initFBO end");
    }

    private void _mallocFBOBuffer(int surfaceWidth, int surfaceHeight) {
        Log.i("LivePusherPlayer", "mallocFBOBuffer in");
        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, mFboId_);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextureid_);
        GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_RGBA, surfaceWidth, surfaceHeight, 0, GLES20.GL_RGBA, GLES20.GL_UNSIGNED_BYTE, null);
        GLES20.glFramebufferTexture2D(GLES20.GL_FRAMEBUFFER, GLES20.GL_COLOR_ATTACHMENT0, GLES20.GL_TEXTURE_2D, mTextureid_, 0);
        if (GLES20.glCheckFramebufferStatus(GLES20.GL_FRAMEBUFFER) != GLES20.GL_FRAMEBUFFER_COMPLETE) {
            Log.e("LivePusherPlayer", "WLCameraRender FBO is wrong!!!");
        }
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 0);
        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);
        Log.i("LivePusherPlayer", "mallocFBOBuffer end");
    }

    private void _releaseOldFBO() {
        Log.i("LivePusherPlayer", "releaseOldFBO in, mFboTextureid_: " + mTextureid_ + " mFboId_: " + mFboId_);
        if (mTextureid_ != 0) {
            int[] textures = {mTextureid_};
            GLES20.glDeleteTextures(1, textures, 0);
            mTextureid_ = 0;
        }
        if (mFboId_ != 0) {
            int[] fbos = {mFboId_};
            GLES20.glDeleteFramebuffers(1, fbos, 0);
            mFboId_ = 0;
        }
        Log.i("LivePusherPlayer", "releaseOldFBO end");
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
