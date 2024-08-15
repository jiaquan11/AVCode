package com.jiaquan.livepusher.camera;

import android.content.Context;
import android.graphics.SurfaceTexture;
import android.opengl.GLES11Ext;
import android.opengl.GLES20;
import android.opengl.Matrix;
import android.util.Log;

import com.jiaquan.livepusher.R;
import com.jiaquan.livepusher.egl.WLEGLSurfaceView;
import com.jiaquan.livepusher.egl.WLShaderUtil;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

public class WLCameraRender implements WLEGLSurfaceView.WLGLRender, SurfaceTexture.OnFrameAvailableListener {
    private final float[] mVertexData_ = {
            -1f, -1f,
            1f, -1f,
            -1f, 1f,
            1f, 1f,
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
    private int mFboTextureid_ = 0;
    private int mCameraTextureid_ = 0;
    private int mUmatrix_;
    private float[] mMatrixArray_ = new float[16];
    private int mVboId_ = 0;
    private int mFboId_ = 0;
    private WLCameraFboRender mWlCameraFboRender_ = null;
    private SurfaceTexture mSurfaceTexture_ = null;
    private int mSurfaceWidth_ = 0;
    private int mSurfaceHeight_ = 0;
    private boolean mCanDrawFrame_ = false;

    public interface OnSurfaceCreateListener {
        void onSurfaceCreate(SurfaceTexture surfaceTexture, int textureid, int surfaceWditdh, int surfaceHeight);
    }

    private OnSurfaceCreateListener mOnSurfaceCreateListener_ = null;

    public void setOnSurfaceCreateListener(OnSurfaceCreateListener onSurfaceCreateListener) {
        mOnSurfaceCreateListener_ = onSurfaceCreateListener;
    }

    public interface OnRenderListener {
        void onRender();
    }

    private OnRenderListener mOnRenderListener_ = null;

    public void setOnRenderListener(OnRenderListener onRenderListener) {
        mOnRenderListener_ = onRenderListener;
    }

    public WLCameraRender(Context context) {
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
        mWlCameraFboRender_ = new WLCameraFboRender(context);
    }

    @Override
    public void onSurfaceCreated() {
        Log.i("LivePusherPlayer", "WLCameraRender onSurfaceCreated in");
        String vertexSource = WLShaderUtil.readRawTxt(mContext_, R.raw.vertex_shader_m);
        String fragmentSource = WLShaderUtil.readRawTxt(mContext_, R.raw.fragment_shader);
        mProgram_ = WLShaderUtil.createProgram(vertexSource, fragmentSource);
        if (mProgram_ > 0) {
            mVPosition_ = GLES20.glGetAttribLocation(mProgram_, "v_Position");
            mFPosition_ = GLES20.glGetAttribLocation(mProgram_, "f_Position");
            mSTexture_ = GLES20.glGetUniformLocation(mProgram_, "sTexture");
            mUmatrix_ = GLES20.glGetUniformLocation(mProgram_, "u_Matrix");
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
            //创建OES扩展纹理
            int[] textureIdsOES = new int[1];
            GLES20.glGenTextures(1, textureIdsOES, 0);
            mCameraTextureid_ = textureIdsOES[0];
            GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, mCameraTextureid_);
            GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_REPEAT);
            GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_REPEAT);
            GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
            GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
            GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, 0);
            mSurfaceTexture_ = new SurfaceTexture(mCameraTextureid_);
            mSurfaceTexture_.setOnFrameAvailableListener(this);
        }
        mWlCameraFboRender_.onCreate();
        Log.i("LivePusherPlayer", "WLCameraRender onSurfaceCreated end");
    }

    @Override
    public void onSurfaceChanged(int width, int height) {
        Log.i("LivePusherPlayer", "WLCameraRender onSurfaceChanged width: " + width + " height: " + height);
        GLES20.glViewport(0, 0, width, height);
        if (mSurfaceWidth_ != width || mSurfaceHeight_ != height) {
            _recreateFBO(width, height);
        }
        mSurfaceWidth_ = width;
        mSurfaceHeight_ = height;
        mSurfaceTexture_.setDefaultBufferSize(width, height);
        mWlCameraFboRender_.onChange(width, height);
        mCanDrawFrame_ = true;
        if (mOnSurfaceCreateListener_ != null) {
            mOnSurfaceCreateListener_.onSurfaceCreate(mSurfaceTexture_, mFboTextureid_, width, height);
        }
    }

    @Override
    public void onDrawFrame() {
        Log.i("LivePusherPlayer", "WLCameraRender onDrawFrame in");
        mSurfaceTexture_.updateTexImage();
        if (!mCanDrawFrame_) {
            Log.w("LivePusherPlayer", "WLCameraRender now can't draw frame");
            return;
        }
        GLES20.glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
        GLES20.glUseProgram(mProgram_);

        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, mFboId_);
        GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, mVboId_);
        GLES20.glEnableVertexAttribArray(mVPosition_);
        GLES20.glVertexAttribPointer(mVPosition_, 2, GLES20.GL_FLOAT, false, 8, 0);
        GLES20.glEnableVertexAttribArray(mFPosition_);
        GLES20.glVertexAttribPointer(mFPosition_, 2, GLES20.GL_FLOAT, false, 8, mVertexData_.length * 4);
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, mCameraTextureid_);
        GLES20.glUniform1i(mSTexture_, 0);
        GLES20.glUniformMatrix4fv(mUmatrix_, 1, false, mMatrixArray_, 0);
        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
        GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, 0);
        GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, 0);
        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);

        mWlCameraFboRender_.onDraw(mFboTextureid_);
        Log.i("LivePusherPlayer", "WLCameraRender onDrawFrame end");
    }

    @Override
    public void onSurfaceDestroy() {
        Log.i("LivePusherPlayer", "WLCameraRender onDestory in");
        GLES20.glDeleteProgram(mProgram_);
        GLES20.glDeleteBuffers(1, new int[]{mFboId_}, 0);
        GLES20.glDeleteBuffers(1, new int[]{mVboId_}, 0);
        GLES20.glDeleteTextures(1, new int[]{mCameraTextureid_}, 0);
        GLES20.glDeleteTextures(1, new int[]{mFboTextureid_}, 0);
        mProgram_ = 0;
        mFboId_ = 0;
        mVboId_ = 0;
        mCameraTextureid_ = 0;
        mFboTextureid_ = 0;
        mWlCameraFboRender_.onDestory();
        mWlCameraFboRender_ = null;
        Log.i("LivePusherPlayer", "WLCameraRender onDestory end");
    }

    @Override
    public void onFrameAvailable(SurfaceTexture surfaceTexture) {
        if (mOnRenderListener_ != null) {
            Log.i("LivePusherPlayer", "WLCameraRender onFrameAvailable");
            mOnRenderListener_.onRender();
        }
    }
    
    public void enableDraw(boolean enable) {
        Log.i("LivePusherPlayer", "WLCameraRender setFrameListenerEnable enable: " + enable);
        mCanDrawFrame_ = enable;
    }

    /**
     * 重置为单位矩阵
     */
    public void resetMatrix() {
        Matrix.setIdentityM(mMatrixArray_, 0);
    }

    /**
     * 设置旋转角度
     */
    public void setAngle(float angle, float x, float y, float z) {
        Matrix.rotateM(mMatrixArray_, 0, angle, x, y, z);
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
        //纹理id生成操作
        int[] textureIds = new int[1];
        GLES20.glGenTextures(1, textureIds, 0);
        mFboTextureid_ = textureIds[0];
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mFboTextureid_);
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
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mFboTextureid_);
        GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_RGBA, surfaceWidth, surfaceHeight, 0, GLES20.GL_RGBA, GLES20.GL_UNSIGNED_BYTE, null);
        GLES20.glFramebufferTexture2D(GLES20.GL_FRAMEBUFFER, GLES20.GL_COLOR_ATTACHMENT0, GLES20.GL_TEXTURE_2D, mFboTextureid_, 0);
        if (GLES20.glCheckFramebufferStatus(GLES20.GL_FRAMEBUFFER) != GLES20.GL_FRAMEBUFFER_COMPLETE) {
            Log.e("LivePusherPlayer", "WLCameraRender FBO is wrong!!!");
        }
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 0);
        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);
        Log.i("LivePusherPlayer", "mallocFBOBuffer end");
    }

    private void _releaseOldFBO() {
        Log.i("LivePusherPlayer", "releaseOldFBO in, mFboTextureid_: " + mFboTextureid_ + " mFboId_: " + mFboId_);
        if (mFboTextureid_ != 0) {
            int[] textures = {mFboTextureid_};
            GLES20.glDeleteTextures(1, textures, 0);
            mFboTextureid_ = 0;
        }
        if (mFboId_ != 0) {
            int[] fbos = {mFboId_};
            GLES20.glDeleteFramebuffers(1, fbos, 0);
            mFboId_ = 0;
        }
        Log.i("LivePusherPlayer", "releaseOldFBO end");
    }
}
