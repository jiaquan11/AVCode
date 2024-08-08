package com.jiaquan.openglesegl;

import static com.jiaquan.openglesegl.WLImageUtil.loadBitmapTexture;

import android.content.Context;
import android.opengl.GLES20;
import android.opengl.Matrix;
import android.util.Log;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

public class WLTextureRender implements WLEGLSurfaceView.WLGLRender {
    private final float[] mVertexData_ = {
          -1f, -1f,
          1f, -1f,
          -1f, 1f,
          1f, 1f,

          -0.5f, -0.5f,
          0.5f, -0.5f,
          -0.5f, 0.5f,
          0.5f, 0.5f
    };
    private final float[] mFragmentData_ = {//FBO坐标，以左下角为原点
            0f, 0f,
            1f, 0f,
            0f, 1f,
            1f, 1f
    };
    private Context mContext_ = null;
    private FloatBuffer mVertexBuffer_ = null;
    private FloatBuffer mFragmentBuffer_ = null;
    private int mProgram_;
    private int mVPosition_;
    private int mFPosition_;
    private int mSTexture_;
    private int mTextureId_;
    private int mUmatrix_;
    private float[] mMatrixArray_ = new float[16];
    private int mVboId_;
    private int mFboId_;
    private TextureInfo mBmpTextureInfo_;
    private TextureInfo mBmpTextureInfo2_;
    private int mSurfaceWidth_;
    private int mSurfaceHeight_;
    private FboRender mFboRender_;

    public interface OnRenderCreateListener{
        void onCreate(int textureId);
    }
    private OnRenderCreateListener mOnRenderCreateListener_ = null;
    public void setOnRenderCreateListener(OnRenderCreateListener onRenderCreateListener) {
        mOnRenderCreateListener_ = onRenderCreateListener;
    }

    public WLTextureRender(Context context) {
        mContext_ = context;
        mFboRender_ = new FboRender(mContext_);
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
        //vertex_shader_m顶点文件支持实现图片的正交投影，缩放等操作(帧缓冲区渲染)
        String vertexSource = WLShaderUtil.readRawTxt(mContext_, R.raw.vertex_shader_m);
        String fragmentSource = WLShaderUtil.readRawTxt(mContext_, R.raw.fragment_shader);
        mProgram_ = WLShaderUtil.createProgram(vertexSource, fragmentSource);
        if (mProgram_ > 0) {
            mVPosition_ = GLES20.glGetAttribLocation(mProgram_, "v_Position");
            mFPosition_ = GLES20.glGetAttribLocation(mProgram_, "f_Position");
            mSTexture_ = GLES20.glGetUniformLocation(mProgram_, "sTexture");
            mUmatrix_ = GLES20.glGetUniformLocation(mProgram_, "u_Matrix");
            /**
             * VBO: Vertex Buffer Object顶点缓冲对象,用于存储顶点数据,
             * 一次性传输到GPU，减少CPU和GPU之间的数据传输
             */
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

            //加载两张图片，并返回图片纹理
            mBmpTextureInfo_ = loadBitmapTexture(mContext_, R.drawable.androids);
            mBmpTextureInfo2_ = loadBitmapTexture(mContext_, R.drawable.ghnl);
            /**
             * FBO Render初始化
             * 这个实例对象是用于将FBO中已处理好的纹理图像绘制到屏幕上
             */
            mFboRender_.onCreate();
        }
    }

    @Override
    public void onSurfaceChanged(int surfaceWidth, int surfaceHeight) {
        Log.i("EGLPlayer", "onSurfaceChanged width: " + surfaceWidth + " height: " + surfaceHeight);
        GLES20.glViewport(0, 0, surfaceWidth, surfaceHeight);
        mSurfaceWidth_ = surfaceWidth;
        mSurfaceHeight_ = surfaceHeight;
        /**
         * FBO初始化
         * 用于离屏渲染，将渲染结果输出到纹理中，而不是输出到屏幕上
         * 这里FBO创建需要在onSurfaceChanged中，因为在onSurfaceCreated中无法获取到SurfaceView的宽高
         * 获取宽高用于给FBO分配内存大小
         */
        _initFBO(surfaceWidth, surfaceHeight);
        mFboRender_.onChange(surfaceWidth, surfaceHeight);
    }

    /**
     * 在FBO的帧缓冲中叠加绘制两张图片纹理
     * 先绘制第一张图片，再绘制第二张图片(单个Suface绘制多个纹理，实现多个纹理叠加效果)
     */
    @Override
    public void onDrawFrame() {
        GLES20.glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);

        GLES20.glUseProgram(mProgram_);
        //绑定FBO
        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, mFboId_);
        //绑定使用VB0
        GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, mVboId_);
        //绘制第一张纹理
        GLES20.glEnableVertexAttribArray(mVPosition_);
        GLES20.glVertexAttribPointer(mVPosition_, 2, GLES20.GL_FLOAT, false, 8, 0);
        GLES20.glEnableVertexAttribArray(mFPosition_);
        GLES20.glVertexAttribPointer(mFPosition_, 2, GLES20.GL_FLOAT, false, 8, mVertexData_.length * 4);
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mBmpTextureInfo_.mTextureId);
        _setMatrix(mSurfaceWidth_, mSurfaceHeight_, mBmpTextureInfo_);//设置图片1的正交投影矩阵
        GLES20.glUniformMatrix4fv(mUmatrix_, 1, false, mMatrixArray_, 0);
        GLES20.glUniform1i(mSTexture_, 0);
        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
        //绘制第二张纹理
        GLES20.glEnableVertexAttribArray(mVPosition_);
        GLES20.glVertexAttribPointer(mVPosition_, 2, GLES20.GL_FLOAT, false, 8, 32);//顶点坐标数组偏移
        GLES20.glEnableVertexAttribArray(mFPosition_);
        GLES20.glVertexAttribPointer(mFPosition_, 2, GLES20.GL_FLOAT, false, 8, mVertexData_.length * 4);
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mBmpTextureInfo2_.mTextureId);
        _setMatrix(mSurfaceWidth_, mSurfaceHeight_, mBmpTextureInfo2_);//设置图片2的正交投影矩阵
        GLES20.glUniformMatrix4fv(mUmatrix_, 1, false, mMatrixArray_, 0);
        GLES20.glUniform1i(mSTexture_, 0);
        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
        //解绑纹理
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 0);
        //解绑VBO
        GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, 0);
        //解绑FBO
        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);

        /**
         * 解绑FBO后，将FBO帧缓冲区中的纹理绘制到屏幕上
         */
        mFboRender_.onDraw(mTextureId_);
    }

    private void _initFBO(int surfaceWidth, int surfaceHeight) {
        /**
         * FBO: Frame Buffer Object帧缓冲对象
         * 用于离屏渲染，将渲染结果输出到纹理中，而不是输出到屏幕上
         */
        //1.创建FBO
        int[] fbos = new int[1];
        GLES20.glGenBuffers(1, fbos, 0);
        mFboId_ = fbos[0];
        //2.绑定FBO
        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, mFboId_);
        int[] textureIds = new int[1];
        GLES20.glGenTextures(1, textureIds, 0);
        mTextureId_ = textureIds[0];
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextureId_);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_REPEAT);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_REPEAT);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
        //3.设置FBO分配内存大小
        GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_RGBA, surfaceWidth, surfaceHeight, 0, GLES20.GL_RGBA, GLES20.GL_UNSIGNED_BYTE, null);
        //4.将纹理绑定到FBO
        GLES20.glFramebufferTexture2D(GLES20.GL_FRAMEBUFFER, GLES20.GL_COLOR_ATTACHMENT0, GLES20.GL_TEXTURE_2D, mTextureId_, 0);
        //5.检查FBO绑定是否成功
        if (GLES20.glCheckFramebufferStatus(GLES20.GL_FRAMEBUFFER) != GLES20.GL_FRAMEBUFFER_COMPLETE) {
            Log.e("EGLPlayer", "Bind FBO is wrong!");
        }
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 0);
        //解绑FBO
        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);
        if (mOnRenderCreateListener_ != null) {
            mOnRenderCreateListener_.onCreate(mTextureId_);//将FBO帧缓冲纹理id传递出去,用于其它SurfaceView控件共享绘制
        }
    }

    public void _setMatrix(int surfaceWidth, int surfaceHeight, TextureInfo textureInfo) {
        float screen_r = surfaceWidth * 1.0f / surfaceHeight;
        float picture_r = textureInfo.mWidth * 1.0f / textureInfo.mHeight;
        if (screen_r > picture_r) {//屏幕宽高比大于图片宽高比
            float r = surfaceWidth / (1.0f * surfaceHeight / textureInfo.mHeight * textureInfo.mWidth);
            Matrix.orthoM(mMatrixArray_, 0, -r, r, -1f, 1f, -1f, 1f);
        } else {//屏幕宽高比小于图片宽高比
            float r = surfaceHeight / (1.0f * surfaceWidth / textureInfo.mWidth * textureInfo.mHeight);
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
