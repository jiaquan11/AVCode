package com.jiaquan.myplayer.opengl;

import android.content.Context;
import android.graphics.SurfaceTexture;
import android.opengl.GLES11Ext;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.view.Surface;

import com.jiaquan.myplayer.R;
import com.jiaquan.myplayer.log.MyLog;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class WLRender implements GLSurfaceView.Renderer, SurfaceTexture.OnFrameAvailableListener {
    public static final int RENDER_NONE = 0;
    public static final int RENDER_YUV = 1;
    public static final int RENDER_MEDIACODEC = 2;

    private Context mContext_ = null;
    private final float[] mVertexData_ = {
//            -1f, 0f,
//            0f, -1f,
//            0f, 1f,
//
//            0f, 1f,
//            0f, -1f,
//            1f, 0f

            -1f, -1f,
            1f, -1f,
            -1f, 1f,
            1f, 1f
    };

    private final float[] mTextureData_ = {
            0f, 1f,
            1f, 1f,
            0f, 0f,
            1f, 0f
//
//            //纹理图像旋转操作
//            1f, 0f,
//            0f, 0f,
//            1f, 1f,
//            0f, 1f
    };

    private int mRenderType_ = RENDER_YUV;//默认为YUV渲染

    private FloatBuffer mVertexBuffer_ = null;
    private FloatBuffer mTextureBuffer_ = null;
    private FloatBuffer mMatrixBuffer_ = null;
    //yuv
    private int mProgramYuv_ = -1;
    private int mAvPositionYuv_ = -1;
    private int mAfPositionYuv_ = -1;
    private int mMatrixYuv_ = -1;

    private int mSamplerY_ = -1;
    private int mSamplerU_ = -1;
    private int mSamplerV_ = -1;

    private int[] mTextureIdYuv_ = null;
    private int mYuvWidth_ = 0;
    private int mYuvHeight_ = 0;
    private ByteBuffer mYBuffer_ = null;
    private ByteBuffer mUBuffer_ = null;
    private ByteBuffer mVBuffer_ = null;

    //mediacodec
    private int mProgramMediacodec_ = -1;
    private int mMatrixMediacodec_ = -1;
    private int mAvPositionMediacodec_ = -1;
    private int mAfPositionMediacodec_ = -1;
    private int mSamplerOESMediacodec_ = -1;
    private int mTextureOESId_ = -1;
    private SurfaceTexture mSurfaceTexture_ = null;
    private Surface mSurface_ = null;

    private int mScreenWidth_ = 0;
    private int mScreenHeight_ = 0;
    private int mPicWdith_ = 0;
    private int mPicHeight_ = 0;
    private float[] mMatrix_ = new float[16];

    public interface OnSurfaceCreateListener {
        void onSurfaceCreate(Surface surface);
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

    public WLRender(Context ctx) {
        MyLog.i("construct WLRender in");
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

        _initMatrix();//单位矩阵初始化
        mMatrixBuffer_ = ByteBuffer.allocateDirect(mMatrix_.length * 4)
                .order(ByteOrder.nativeOrder())
                .asFloatBuffer()
                .put(mMatrix_);
        mMatrixBuffer_.position(0);
        MyLog.i("construct WLRender end");
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        MyLog.i("WLRender onSurfaceCreated in");
        //初始化YUV格式绘制
        _initRenderYUV();
        //初始化硬解Surface图像绘制
        _initRenderMediaCodec();
        MyLog.i("WLRender onSurfaceCreated end");
    }

    /**
     * 屏幕尺寸发生变化时调用，例如横竖屏切换
     * @param gl the GL interface. Use <code>instanceof</code> to
     * test if the interface supports GL11 or higher interfaces.
     * @param width
     * @param height
     */
    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        GLES20.glViewport(0, 0, width, height);
        mScreenWidth_ = width;
        mScreenHeight_ = height;
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
        GLES20.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);//0 0 0是黑色清屏 1 1 1是白色
        if (mRenderType_ == RENDER_YUV) {
            _renderYUV();
        } else if (mRenderType_ == RENDER_MEDIACODEC) {
            _renderMediaCodec();
        }
    }

    /*
     * 当硬解出画面数据时，会存放在surface这个屏幕缓冲区中，他然后就会给到
     * SurfaceTexture中，SurfaceTexture会马上回调onFrameAvailable，表示有有效实际
     * 数据画面到达，那么接着就可以进行渲染操作，调用一下requestRender请求onDrawFrame,
     * 在onDrawFrame中surfaceTexture.updateTexImage();表示将缓存数据刷到前台更新，同时
     * 使用opengl的操作绘制出来，最后GLSurfaceView自己内部封装的渲染操作渲染出来
     * */
    @Override
    public void onFrameAvailable(SurfaceTexture surfaceTexture) {
        if (mOnRenderListener_ != null) {
            mOnRenderListener_.onRender();
        }
    }

    /**
     * 设置渲染类型
     * @param renderType
     */
    public void setRenderType(int renderType) {
        mRenderType_ = renderType;
    }

    /**
     * 设置视频宽高
     * @param width
     * @param height
     */
    public void setVideoSize(int width, int height) {
        mPicWdith_ = width;
        mPicHeight_ = height;
    }

    public void setYUVRenderData(int width, int height, byte[] y, byte[] u, byte[] v) {
        mYuvWidth_ = width;
        mYuvHeight_ = height;
        mYBuffer_ = ByteBuffer.wrap(y);
        mUBuffer_ = ByteBuffer.wrap(u);
        mVBuffer_ = ByteBuffer.wrap(v);
    }

    private void _initRenderYUV() {
        String vertexSource = WLShaderUtil.readRawTxt(mContext_, R.raw.vertex_shader);
        String fragmentSource = WLShaderUtil.readRawTxt(mContext_, R.raw.fragment_yuv);
        mProgramYuv_ = WLShaderUtil.createProgram(vertexSource, fragmentSource);
        mAvPositionYuv_ = GLES20.glGetAttribLocation(mProgramYuv_, "av_Position");
        mAfPositionYuv_ = GLES20.glGetAttribLocation(mProgramYuv_, "af_Position");
        mMatrixYuv_ = GLES20.glGetUniformLocation(mProgramYuv_, "u_Matrix");
        mSamplerY_ = GLES20.glGetUniformLocation(mProgramYuv_, "sampler_y");
        mSamplerU_ = GLES20.glGetUniformLocation(mProgramYuv_, "sampler_u");
        mSamplerV_ = GLES20.glGetUniformLocation(mProgramYuv_, "sampler_v");
        //1、生成三个纹理对象(纹理id是纹理对象的标识)
        mTextureIdYuv_ = new int[3];
        GLES20.glGenTextures(3, mTextureIdYuv_, 0);
        for (int i = 0; i < 3; i++) {
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextureIdYuv_[i]);
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_REPEAT);
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_REPEAT);
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, i);
        }
    }

    private void _renderYUV() {
        if ((mYuvWidth_ > 0) && (mYuvHeight_ > 0) && (mYBuffer_ != null) && (mUBuffer_ != null) && (mVBuffer_ != null)) {
            GLES20.glUseProgram(mProgramYuv_);

            GLES20.glEnableVertexAttribArray(mAvPositionYuv_);
            GLES20.glVertexAttribPointer(mAvPositionYuv_, 2, GLES20.GL_FLOAT, false, 8, mVertexBuffer_);
            GLES20.glEnableVertexAttribArray(mAfPositionYuv_);
            GLES20.glVertexAttribPointer(mAfPositionYuv_, 2, GLES20.GL_FLOAT, false, 8, mTextureBuffer_);
            //2、将纹理对象绑定到激活的纹理单元,这里需要绑定三个纹理单元(Y,U,V)
            GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextureIdYuv_[0]);
            GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_LUMINANCE, mYuvWidth_, mYuvHeight_, 0, GLES20.GL_LUMINANCE, GLES20.GL_UNSIGNED_BYTE, mYBuffer_);
            GLES20.glActiveTexture(GLES20.GL_TEXTURE1);
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextureIdYuv_[1]);
            GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_LUMINANCE, mYuvWidth_ / 2, mYuvHeight_ / 2, 0, GLES20.GL_LUMINANCE, GLES20.GL_UNSIGNED_BYTE, mUBuffer_);
            GLES20.glActiveTexture(GLES20.GL_TEXTURE2);
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextureIdYuv_[2]);
            GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_LUMINANCE, mYuvWidth_ / 2, mYuvHeight_ / 2, 0, GLES20.GL_LUMINANCE, GLES20.GL_UNSIGNED_BYTE, mVBuffer_);
            //3、将纹理单元绑定到着色器采样器，这里需要绑定三个纹理采样器
            GLES20.glUniform1i(mSamplerY_, 0);
            GLES20.glUniform1i(mSamplerU_, 1);
            GLES20.glUniform1i(mSamplerV_, 2);
            /**
             * 给矩阵变量赋值
             * 这里每次渲染都设置一次矩阵,是为了解决glUniformMatrix4fv概率崩溃的问题，目前原因未知。
             * 本来是不需要每次都设置矩阵的，只需要设置一次就可以了
             */
            _setMatrix(mScreenWidth_, mScreenHeight_, mYuvWidth_, mYuvHeight_);
            GLES20.glUniformMatrix4fv(mMatrixYuv_, 1, false, mMatrixBuffer_);
            int error = GLES20.glGetError();
            if (error != GLES20.GL_NO_ERROR) {
                MyLog.e("OpenGL matrix error: " + error);
                throw new RuntimeException("OpenGL matrix error: " + error);
            }
            GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 0);
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 1);
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 2);
            mYBuffer_.clear();
            mUBuffer_.clear();
            mVBuffer_.clear();
            mYBuffer_ = null;
            mUBuffer_ = null;
            mVBuffer_ = null;
        }
    }

    private void _initRenderMediaCodec() {
        MyLog.i("initRenderMediaCodec in");
        String vertexSource = WLShaderUtil.readRawTxt(mContext_, R.raw.vertex_shader);
        String fragmentSource = WLShaderUtil.readRawTxt(mContext_, R.raw.fragment_mediacodec);
        mProgramMediacodec_ = WLShaderUtil.createProgram(vertexSource, fragmentSource);
        mAvPositionMediacodec_ = GLES20.glGetAttribLocation(mProgramMediacodec_, "av_Position");
        mAfPositionMediacodec_ = GLES20.glGetAttribLocation(mProgramMediacodec_, "af_Position");
        mSamplerOESMediacodec_ = GLES20.glGetUniformLocation(mProgramMediacodec_, "sTexture");
        mMatrixMediacodec_ = GLES20.glGetUniformLocation(mProgramMediacodec_, "u_Matrix");
        //1.生成并设置纹理对象(纹理id是纹理对象的标识)
        //创建一个OES纹理
        int[] textrueids = new int[1];
        GLES20.glGenTextures(1, textrueids, 0);
        mTextureOESId_ = textrueids[0];
        GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, mTextureOESId_);
        GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_REPEAT);
        GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_REPEAT);
        GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
        GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
        GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, 0);
        mSurfaceTexture_ = new SurfaceTexture(mTextureOESId_);
        mSurface_ = new Surface(mSurfaceTexture_);
        mSurfaceTexture_.setOnFrameAvailableListener(this);
        if (mOnSurfaceCreateListener_ != null) {//只有硬解才会需要回调surface
            mOnSurfaceCreateListener_.onSurfaceCreate(mSurface_);
        }
        MyLog.i("initRenderMediaCodec out");
    }

    private void _renderMediaCodec() {
        mSurfaceTexture_.updateTexImage();//将缓存数据刷到前台更新
        GLES20.glUseProgram(mProgramMediacodec_);
        int error = GLES20.glGetError();
        if (error != GLES20.GL_NO_ERROR) {
            throw new RuntimeException("OpenGL use program error: " + error);
        }
        GLES20.glEnableVertexAttribArray(mAvPositionMediacodec_);
        GLES20.glVertexAttribPointer(mAvPositionMediacodec_, 2, GLES20.GL_FLOAT, false, 8, mVertexBuffer_);
        GLES20.glEnableVertexAttribArray(mAfPositionMediacodec_);
        GLES20.glVertexAttribPointer(mAfPositionMediacodec_, 2, GLES20.GL_FLOAT, false, 8, mTextureBuffer_);
        //2、将纹理对象绑定到激活的纹理单元
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, mTextureOESId_);
        //3、将纹理单元绑定到着色器采样器
        GLES20.glUniform1i(mSamplerOESMediacodec_, 0);//给纹理采样器赋值
        /**
         * 给矩阵变量赋值
         * 这里每次渲染都设置一次矩阵,是为了解决glUniformMatrix4fv概率崩溃的问题，目前原因未知。
         * 本来是不需要每次都设置矩阵的，只需要设置一次就可以了
         */
        _setMatrix(mScreenWidth_, mScreenHeight_, mPicWdith_, mPicHeight_);
        GLES20.glUniformMatrix4fv(mMatrixMediacodec_, 1, false, mMatrixBuffer_);//给矩阵变量赋值
        error = GLES20.glGetError();
        if (error != GLES20.GL_NO_ERROR) {
            MyLog.e("OpenGL matrix error: " + error);
            throw new RuntimeException("OpenGL matrix error: " + error);
        }
        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
        GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, 0);
    }

    //初始化单位矩阵
    private void _initMatrix() {//4*4矩阵，对角线数字为1，其余为0
        for (int i = 0; i < 16; ++i) {
            if (i % 5 == 0) {
                mMatrix_[i] = 1.0f;
            } else {
                mMatrix_[i] = 0.0f;
            }
        }
    }

    /**
     * 设置矩阵，用来进行缩放等操作
     * @param screen_width 屏幕宽度
     * @param screen_height 屏幕高度
     * @param pic_width 图片宽度
     * @param pic_height 图片高度
     */
    private void _setMatrix(int screen_width, int screen_height, int pic_width, int pic_height) {
        //屏幕720*1280 图片:517*685
        float screen_r = 1.0f * screen_width / screen_height;
        float picture_r = 1.0f * pic_width / pic_height;
        if (screen_r > picture_r) {//图片宽度缩放
            float r = screen_width / (1.0f * screen_height / pic_height * pic_width);
            _orthoM(-r, r, -1, 1, mMatrix_);
        } else {//图片宽的比率大于屏幕，则宽进行直接覆盖屏幕，而图片高度缩放
            float r = screen_height / (1.0f * screen_width / pic_width * pic_height);
            _orthoM(-1, 1, -r, r, mMatrix_);
        }

        if(mMatrix_.length != 16) {
            throw new IllegalArgumentException("Matrix must have 16 elements.");
        }
        mMatrixBuffer_.put(mMatrix_);
        mMatrixBuffer_.position(0);
    }

    /**
     * 正交投影矩阵
     * @param left
     * @param right
     * @param bottom
     * @param top
     * @param matrix
     */
    private void _orthoM(float left, float right, float bottom, float top, float []matrix) {
        matrix[0] = 2 / (right - left);
        matrix[3] = (right + left) / (right - left) * -1;
        matrix[5] = 2 / (top - bottom);
        matrix[7] = (top + bottom) / (top - bottom) * -1;
        matrix[10] = 1;
        matrix[11] = 1;
    }
}
