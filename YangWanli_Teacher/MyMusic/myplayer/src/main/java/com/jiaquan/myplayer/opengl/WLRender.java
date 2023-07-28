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
    private final String TAG = WLRender.class.getSimpleName();

    private Context context = null;

    public static final int RENDER_YUV = 1;
    public static final int RENDER_MEDIACODEC = 2;

    private final float[] vertexData = {
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

    private final float[] textureData = {
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

    private int renderType = RENDER_YUV;//默认为YUV渲染

    private FloatBuffer vertexBuffer;
    private FloatBuffer textureBuffer;
    private FloatBuffer matrixBuffer;
    //yuv
    private int program_yuv;
    private int avPosition_yuv;
    private int afPosition_yuv;
    private int u_matrix_yuv;

    private int sampler_y;
    private int sampler_u;
    private int sampler_v;

    private int[] textureId_yuv;
    private int mYuvWidth = 0;
    private int mYuvHeight = 0;
    private ByteBuffer y;
    private ByteBuffer u;
    private ByteBuffer v;

    //mediacodec
    private int program_mediacodec = -1;
    private int u_matrix_mediacodec = -1;
    private int avPosition_mediacodec = -1;
    private int afPosition_mediacodec = -1;
    private int samplerOES_mediacodec = -1;
    private int textureId_mediacodec = -1;
    private SurfaceTexture surfaceTexture = null;
    private Surface surface = null;

    private int mScreenWidth = 0;
    private int mScreenHeight = 0;
    private int mPicWdith = 0;
    private int mPicHeight = 0;
    private float[] matrix = new float[16];

    public interface OnSurfaceCreateListener {
        void onSurfaceCreate(Surface surface);
    }
    private OnSurfaceCreateListener onSurfaceCreateListener = null;
    public void setOnSurfaceCreateListener(OnSurfaceCreateListener onSurfaceCreateListener) {
        this.onSurfaceCreateListener = onSurfaceCreateListener;
    }

    private OnRenderListener onRenderListener = null;
    public interface OnRenderListener {
        void onRender();
    }
    public void setOnRenderListener(OnRenderListener onRenderListener) {
        this.onRenderListener = onRenderListener;
    }

    public WLRender(Context ctx) {
        MyLog.i("construct WLRender in");
        context = ctx;

        initMatrix();//单位矩阵初始化

        vertexBuffer = ByteBuffer.allocateDirect(vertexData.length * 4)
                .order(ByteOrder.nativeOrder())
                .asFloatBuffer()
                .put(vertexData);
        vertexBuffer.position(0);

        textureBuffer = ByteBuffer.allocateDirect(textureData.length * 4)
                .order(ByteOrder.nativeOrder())
                .asFloatBuffer()
                .put(textureData);
        textureBuffer.position(0);

        matrixBuffer = ByteBuffer.allocateDirect(matrix.length * 4)
                .order(ByteOrder.nativeOrder())
                .asFloatBuffer()
                .put(matrix);
        matrixBuffer.position(0);
        MyLog.i("construct WLRender end");
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        MyLog.i("WLRender onSurfaceCreated in");
        //不管哪种渲染方式，都先做好初始化
        //初始化渲染YUV
        initRenderYUV();

        //初始化硬解渲染画面
        initRenderMediaCodec();
        MyLog.i("WLRender onSurfaceCreated end");
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        GLES20.glViewport(0, 0, width, height);
        mScreenWidth = width;
        mScreenHeight = height;
    }

    /*
     * GLSurfaceView自己内部封装了渲染操作，当在onDrawFrame中 opengl绘制完成之后，就自动渲染画面出来了
     * */
    @Override
    public void onDrawFrame(GL10 gl) {
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
        GLES20.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);//0 0 0是黑色清屏 1 1 1是白色

        if (renderType == RENDER_YUV) {
            renderYUV();
        } else if (renderType == RENDER_MEDIACODEC) {
            renderMediaCodec();
        }

        //将绘制操作放在外面，每次都会进行绘制一次，暂时解决概率性的闪屏问题
        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
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
        if (onRenderListener != null) {
//            MyLog.i("onFrameAvailable in");
            onRenderListener.onRender();
//            MyLog.i("onFrameAvailable out");
        }

        setMatrix(mScreenWidth, mScreenHeight, mPicWdith, mPicHeight);
    }

    public void setRenderType(int renderType) {
        this.renderType = renderType;
    }

    public void setVideoSize(int width, int height) {
        mPicWdith = width;
        mPicHeight = height;
    }

    public void setYUVRenderData(int width, int height, byte[] y, byte[] u, byte[] v) {
        this.mYuvWidth = width;
        this.mYuvHeight = height;
        this.y = ByteBuffer.wrap(y);
        this.u = ByteBuffer.wrap(u);
        this.v = ByteBuffer.wrap(v);

        setMatrix(mScreenWidth, mScreenHeight, mYuvWidth, mYuvHeight);
    }

    private void initRenderYUV() {
        String vertexSource = WLShaderUtil.readRawTxt(context, R.raw.vertex_shader);
        String fragmentSource = WLShaderUtil.readRawTxt(context, R.raw.fragment_yuv);
        program_yuv = WLShaderUtil.createProgram(vertexSource, fragmentSource);
        u_matrix_yuv = GLES20.glGetUniformLocation(program_yuv, "u_Matrix");
        avPosition_yuv = GLES20.glGetAttribLocation(program_yuv, "av_Position");
        afPosition_yuv = GLES20.glGetAttribLocation(program_yuv, "af_Position");

        sampler_y = GLES20.glGetUniformLocation(program_yuv, "sampler_y");
        sampler_u = GLES20.glGetUniformLocation(program_yuv, "sampler_u");
        sampler_v = GLES20.glGetUniformLocation(program_yuv, "sampler_v");

        textureId_yuv = new int[3];
        GLES20.glGenTextures(3, textureId_yuv, 0);

        for (int i = 0; i < 3; i++) {
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textureId_yuv[i]);
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_REPEAT);
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_REPEAT);
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
        }
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 0);
    }

    private void renderYUV() {
        if ((mYuvWidth > 0) && (mYuvHeight > 0) && (y != null) && (u != null) && (v != null)) {
            GLES20.glUseProgram(program_yuv);

            GLES20.glUniformMatrix4fv(u_matrix_yuv, 1, false, matrixBuffer);//给矩阵变量赋值

            GLES20.glEnableVertexAttribArray(avPosition_yuv);
            GLES20.glVertexAttribPointer(avPosition_yuv, 2, GLES20.GL_FLOAT, false, 8, vertexBuffer);

            GLES20.glEnableVertexAttribArray(afPosition_yuv);
            GLES20.glVertexAttribPointer(afPosition_yuv, 2, GLES20.GL_FLOAT, false, 8, textureBuffer);

            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textureId_yuv[0]);
            GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
            GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_LUMINANCE, mYuvWidth, mYuvHeight, 0, GLES20.GL_LUMINANCE, GLES20.GL_UNSIGNED_BYTE, y);

            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textureId_yuv[1]);
            GLES20.glActiveTexture(GLES20.GL_TEXTURE1);
            GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_LUMINANCE, mYuvWidth / 2, mYuvHeight / 2, 0, GLES20.GL_LUMINANCE, GLES20.GL_UNSIGNED_BYTE, u);

            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textureId_yuv[2]);
            GLES20.glActiveTexture(GLES20.GL_TEXTURE2);
            GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_LUMINANCE, mYuvWidth / 2, mYuvHeight / 2, 0, GLES20.GL_LUMINANCE, GLES20.GL_UNSIGNED_BYTE, v);

            GLES20.glUniform1i(sampler_y, 0);
            GLES20.glUniform1i(sampler_u, 1);
            GLES20.glUniform1i(sampler_v, 2);

            y.clear();
            u.clear();
            v.clear();
            y = null;
            u = null;
            v = null;
//            GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
//            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 0);
        }
    }

    private void initRenderMediaCodec() {
        MyLog.i("initRenderMediaCodec in");
        String vertexSource = WLShaderUtil.readRawTxt(context, R.raw.vertex_shader);
        String fragmentSource = WLShaderUtil.readRawTxt(context, R.raw.fragment_mediacodec);
        program_mediacodec = WLShaderUtil.createProgram(vertexSource, fragmentSource);
        u_matrix_mediacodec = GLES20.glGetUniformLocation(program_mediacodec, "u_Matrix");
        avPosition_mediacodec = GLES20.glGetAttribLocation(program_mediacodec, "av_Position");
        afPosition_mediacodec = GLES20.glGetAttribLocation(program_mediacodec, "af_Position");
        samplerOES_mediacodec = GLES20.glGetUniformLocation(program_mediacodec, "sTexture");

        int[] textrueids = new int[1];
        GLES20.glGenTextures(1, textrueids, 0);
        textureId_mediacodec = textrueids[0];
        GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, textureId_mediacodec);
        GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_REPEAT);
        GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_REPEAT);
        GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
        GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
        GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, 0);

        surfaceTexture = new SurfaceTexture(textureId_mediacodec);
        surface = new Surface(surfaceTexture);
        surfaceTexture.setOnFrameAvailableListener(this);

        if (onSurfaceCreateListener != null) {//只有硬解才会需要回调surface
            onSurfaceCreateListener.onSurfaceCreate(surface);
        }
        MyLog.i("initRenderMediaCodec out");
    }

    private void renderMediaCodec() {
        MyLog.i("renderMediaCodec in");
        //先更新surfaceTexture缓存数据，然后下面进行opengl绘制，最后由GLSurfaceView内部的EGL环境(swapBuffe方法)进行交换输出显示出来
        surfaceTexture.updateTexImage();

        GLES20.glUseProgram(program_mediacodec);

        GLES20.glUniformMatrix4fv(u_matrix_mediacodec, 1, false, matrixBuffer);//给矩阵变量赋值

        GLES20.glEnableVertexAttribArray(avPosition_mediacodec);
        GLES20.glVertexAttribPointer(avPosition_mediacodec, 2, GLES20.GL_FLOAT, false, 8, vertexBuffer);

        GLES20.glEnableVertexAttribArray(afPosition_mediacodec);
        GLES20.glVertexAttribPointer(afPosition_mediacodec, 2, GLES20.GL_FLOAT, false, 8, textureBuffer);

        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, textureId_mediacodec);

        GLES20.glUniform1i(samplerOES_mediacodec, 0);
//        GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, 0);
        MyLog.i("renderMediaCodec out");
    }

    private void initMatrix() {//4*4矩阵，对角线数字为1，其余为0
        for (int i = 0; i < 16; ++i) {
            if (i % 5 == 0) {
                matrix[i] = 1.0f;
            } else {
                matrix[i] = 0.0f;
            }
        }
    }

    private void setMatrix(int screen_width, int screen_height, int pic_width, int pic_height) {
        //屏幕720*1280 图片:517*685
        float screen_r = 1.0f * screen_width / screen_height;
        float picture_r = 1.0f * pic_width / pic_height;
        if (screen_r > picture_r) {//图片宽度缩放
            float r = screen_width / (1.0f * screen_height / pic_height * pic_width);
            orthoM(-r, r, -1, 1, matrix);
        } else {//图片宽的比率大于屏幕，则宽进行直接覆盖屏幕，而图片高度缩放
            float r = screen_height / (1.0f * screen_width / pic_width * pic_height);
            orthoM(-1, 1, -r, r, matrix);
        }

        matrixBuffer.put(matrix);
        matrixBuffer.position(0);
    }

    /**
     * 正交投影
     */
    private void orthoM(float left, float right, float bottom, float top, float []matrix) {
        matrix[0] = 2 / (right - left);
        matrix[3] = (right + left) / (right - left) * -1;
        matrix[5] = 2 / (top - bottom);
        matrix[7] = (top + bottom) / (top - bottom) * -1;
        matrix[10] = 1;
        matrix[11] = 1;
    }
}
