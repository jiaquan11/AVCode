package com.jiaquan.livepusher.encodec;

import android.content.Context;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.media.MediaMuxer;
import android.util.Log;
import android.view.Surface;

import com.jiaquan.livepusher.egl.EglHelper;
import com.jiaquan.livepusher.egl.WLEGLSurfaceView;

import java.io.IOException;
import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;

import javax.microedition.khronos.egl.EGLContext;

public abstract class WLBaseMediaEncoder {
    public final static int RENDERMODE_WHEN_DIRTY = 0;
    public final static int RENDERMODE_CONTINUOUSLY = 1;
    private Surface mSurface_ = null;
    private EGLContext mEglContext_ = null;
    private int mWidth_ = -1;
    private int mHeight_ = -1;
    private MediaCodec mVideoEncoder_ = null;
    private MediaFormat mVideoFormat_ = null;
    private MediaCodec.BufferInfo mVideoBufferInfo_ = null;
    private MediaCodec mAudioEncoder_ = null;
    private MediaFormat mAudioFormat_ = null;
    private MediaCodec.BufferInfo mAudioBufferInfo_ = null;
    private long mAudioPts_ = 0;
    private int mSampleRate_ = 0;
    private MediaMuxer mMediaMuxer_ = null;
    private boolean mEncoderStart_;
    private boolean mAudioExit_;
    private boolean mVideoExit_;
    private WLEGLMediaThread mWleglMediaThread_ = null;
    private VideoEncoderThread mVideoEncoderThread_ = null;
    private AudioEncoderThread mAudioEncoderThread_ = null;
    private WLEGLSurfaceView.WLGLRender mWlglRender_ = null;
    private int mRenderMode_ = RENDERMODE_CONTINUOUSLY;

    public interface OnMediaInfoListener {
        void onMediaTime(int times);
    }
    private OnMediaInfoListener mOnMediaInfoListener_ = null;
    public void setOnMediaInfoListener(OnMediaInfoListener onMediaInfoListener) {
        mOnMediaInfoListener_ = onMediaInfoListener;
    }

    public WLBaseMediaEncoder(Context context) {

    }

    public void setSurfaceAndEglContext(Surface surface, EGLContext eglContext) {
        mSurface_ = surface;
        mEglContext_ = eglContext;
    }

    public void setRender(WLEGLSurfaceView.WLGLRender wlglRender) {
        mWlglRender_ = wlglRender;
    }

    public void setRenderMode(int renderMode) {
        if (mWlglRender_ == null) {
            throw new RuntimeException("must set render before");
        }
        mRenderMode_ = renderMode;
    }

    public void requestRender() {
        if (mWleglMediaThread_ != null) {
            mWleglMediaThread_.requestRender();
        }
    }

    public void initEncoder(String savePath, int width, int height, int sampleRate, int channelCount) {
        mWidth_ = width;
        mHeight_ = height;
        initMediaEncoder(savePath, width, height, sampleRate, channelCount);
    }

    public void startRecord() {
        if ((mSurface_ != null) && (mEglContext_ != null)) {
            mAudioExit_ = false;
            mVideoExit_ = false;
            mEncoderStart_ = false;
            mAudioPts_ = 0;
            mWleglMediaThread_ = new WLEGLMediaThread(new WeakReference<WLBaseMediaEncoder>(this));
            mVideoEncoderThread_ = new VideoEncoderThread(new WeakReference<WLBaseMediaEncoder>(this));
            mAudioEncoderThread_ = new AudioEncoderThread(new WeakReference<WLBaseMediaEncoder>(this));
            mWleglMediaThread_.mIsCreate_ = true;
            mWleglMediaThread_.mIsChange_ = true;

            mWleglMediaThread_.start();
            mVideoEncoderThread_.start();
            mAudioEncoderThread_.start();
        }
    }

    public void stopRecord() {
        if ((mWleglMediaThread_ != null) && (mVideoEncoderThread_ != null) && (mAudioEncoderThread_ != null)) {
            mVideoEncoderThread_.exit();
            mAudioEncoderThread_.exit();
            mWleglMediaThread_.onDestroy();
            mVideoEncoderThread_ = null;
            mAudioEncoderThread_ = null;
            mWleglMediaThread_ = null;
        }
    }

    private void initMediaEncoder(String savePath, int width, int height, int sampleRate, int channelCount) {
        try {
            mMediaMuxer_ = new MediaMuxer(savePath, MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4);
            initVideoEncoder(MediaFormat.MIMETYPE_VIDEO_AVC, width, height);
            initAudioEncoder(MediaFormat.MIMETYPE_AUDIO_AAC, sampleRate, channelCount);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private void initAudioEncoder(String mimeType, int sampleRate, int channelCount) {
        try {
            mSampleRate_ = sampleRate;
            mAudioBufferInfo_ = new MediaCodec.BufferInfo();
            mAudioFormat_ = MediaFormat.createAudioFormat(mimeType, sampleRate, channelCount);
            mAudioFormat_.setInteger(MediaFormat.KEY_BIT_RATE, 96000);
            mAudioFormat_.setInteger(MediaFormat.KEY_AAC_PROFILE, MediaCodecInfo.CodecProfileLevel.AACObjectLC);
            mAudioFormat_.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, 4096);
            mAudioEncoder_ = MediaCodec.createEncoderByType(mimeType);
            mAudioEncoder_.configure(mAudioFormat_, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
        } catch (IOException e) {
            e.printStackTrace();
            mAudioBufferInfo_ = null;
            mAudioFormat_ = null;
            mAudioEncoder_ = null;
        }
    }

    private void initVideoEncoder(String mimeType, int width, int height) {
        Log.i("LivePusherPlayer", "initVideoEncoder: width: " + width + ", height: " + height);
        try {
            mVideoBufferInfo_ = new MediaCodec.BufferInfo();
            mVideoFormat_ = MediaFormat.createVideoFormat(mimeType, width, height);
            mVideoFormat_.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface);
            mVideoFormat_.setInteger(MediaFormat.KEY_BIT_RATE, width * height * 4);
            mVideoFormat_.setInteger(MediaFormat.KEY_FRAME_RATE, 30);
            mVideoFormat_.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, 1);
            mVideoEncoder_ = MediaCodec.createEncoderByType(mimeType);
            Log.i("LivePusherPlayer", "initVideoEncoder: mVideoFormat_: " + mVideoFormat_);
            mVideoEncoder_.configure(mVideoFormat_, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
            mSurface_ = mVideoEncoder_.createInputSurface();
        } catch (IOException e) {
            e.printStackTrace();
            mVideoEncoder_ = null;
            mVideoFormat_ = null;
            mVideoBufferInfo_ = null;
        }
    }

    public void putPCMData(byte[] buffer, int size) {
        if ((mAudioEncoderThread_ != null) && !mAudioEncoderThread_.isExit && (buffer != null) && (size > 0)) {
            int inputBufferIndex = mAudioEncoder_.dequeueInputBuffer(0);
            if (inputBufferIndex >= 0) {
                ByteBuffer byteBuffer = mAudioEncoder_.getInputBuffers()[inputBufferIndex];
                byteBuffer.clear();
                byteBuffer.put(buffer);
                long pts = getAudioPts(size, mSampleRate_);
                mAudioEncoder_.queueInputBuffer(inputBufferIndex, 0, size, pts, 0);
            }
        }
    }

    static class WLEGLMediaThread extends Thread {
        private WeakReference<WLBaseMediaEncoder> mEncoder_;
        private EglHelper mEglHelper_;
        private Object mObject_;
        private boolean mIsExit_ = false;
        private boolean mIsCreate_ = false;
        private boolean mIsChange_ = false;
        private boolean mIsStart_ = false;
        public WLEGLMediaThread(WeakReference<WLBaseMediaEncoder> encoder) {
            mEncoder_ = encoder;
        }

        @Override
        public void run() {
            super.run();
            mIsExit_ = false;
            mIsStart_ = false;
            mObject_ = new Object();
            mEglHelper_ = new EglHelper();
            mEglHelper_.initEgl(mEncoder_.get().mSurface_, mEncoder_.get().mEglContext_);
            while (true) {
                if (mIsExit_) {
                    release();
                    break;
                }
                if (mIsStart_) {
                    if (mEncoder_.get().mRenderMode_ == RENDERMODE_WHEN_DIRTY) {
                        synchronized (mObject_) {
                            try {
                                mObject_.wait();
                            } catch (InterruptedException e) {
                                e.printStackTrace();
                            }
                        }
                    } else if (mEncoder_.get().mRenderMode_ == RENDERMODE_CONTINUOUSLY) {
                        try {
                            Thread.sleep(40);//25帧的帧率，每帧40毫秒
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                        }
                    } else {
                        throw new RuntimeException("mRenderMode is wrong value");
                    }
                }
                onCreate();
                onChange(mEncoder_.get().mWidth_, mEncoder_.get().mHeight_);
                onDraw();
                mIsStart_ = true;
            }
        }

        private void onCreate() {
            if (mIsCreate_ && mEncoder_.get().mWlglRender_ != null) {
                mIsCreate_ = false;
                mEncoder_.get().mWlglRender_.onSurfaceCreated();
            }
        }

        private void onChange(int width, int height) {
            if (mIsChange_ && mEncoder_.get().mWlglRender_ != null) {
                mIsChange_ = false;
                mEncoder_.get().mWlglRender_.onSurfaceChanged(width, height);
            }
        }

        private void onDraw() {
            if ((mEncoder_.get().mWlglRender_ != null) && (mEglHelper_ != null) && mIsStart_) {
                mEncoder_.get().mWlglRender_.onDrawFrame();
                mEglHelper_.swapBuffers();
            }
        }

        public void onDestroy() {
            mIsExit_ = true;
            requestRender();
        }

        private void requestRender() {
            if (mObject_ != null) {
                synchronized (mObject_) {
                    mObject_.notifyAll();
                }
            }
        }

        public void release() {
            if (mEglHelper_ != null) {
                mEglHelper_.destroyEgl();
                mEglHelper_ = null;
                mObject_ = null;
                mEncoder_ = null;
            }
        }
    }

    static class VideoEncoderThread extends Thread {
        private WeakReference<WLBaseMediaEncoder> encoder = null;
        private boolean isExit;
        private MediaCodec videoEncoder = null;
        private MediaCodec.BufferInfo videoBufferInfo = null;
        private MediaMuxer mediaMuxer = null;
        private long pts;
        private int videoTrackIndex = -1;
        VideoEncoderThread(WeakReference<WLBaseMediaEncoder> encoder) {
            this.encoder = encoder;
            videoEncoder = encoder.get().mVideoEncoder_;
            videoBufferInfo = encoder.get().mVideoBufferInfo_;
            mediaMuxer = encoder.get().mMediaMuxer_;
            videoTrackIndex = -1;
        }

        @Override
        public void run() {
            super.run();
            pts = 0;
            videoTrackIndex = -1;
            isExit = false;
            videoEncoder.start();
            while (true) {
                if (isExit) {
                    videoEncoder.stop();
                    videoEncoder.release();
                    videoEncoder = null;
                    encoder.get().mVideoExit_ = true;
                    if (encoder.get().mAudioExit_) {
                        mediaMuxer.stop();
                        mediaMuxer.release();
                        mediaMuxer = null;
                        Log.i("LivePusherPlayer", "Video 录制完成!!!");
                    }
                    break;
                }

                int outputBufferIndex = videoEncoder.dequeueOutputBuffer(videoBufferInfo, 0);
                if (outputBufferIndex == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                    videoTrackIndex = mediaMuxer.addTrack(videoEncoder.getOutputFormat());
                    if (encoder.get().mAudioEncoderThread_.audioTrackIndex != -1) {
                        mediaMuxer.start();
                        encoder.get().mEncoderStart_ = true;
                    }
                } else {
                    while (outputBufferIndex >= 0) {
                        if (encoder.get().mEncoderStart_) {
                            ByteBuffer outputBuffer = videoEncoder.getOutputBuffers()[outputBufferIndex];
                            outputBuffer.position(videoBufferInfo.offset);
                            outputBuffer.limit(videoBufferInfo.offset + videoBufferInfo.size);

                            if (pts == 0) {
                                pts = videoBufferInfo.presentationTimeUs;
                            }
                            videoBufferInfo.presentationTimeUs = videoBufferInfo.presentationTimeUs - pts;

                            mediaMuxer.writeSampleData(videoTrackIndex, outputBuffer, videoBufferInfo);
                            if (encoder.get().mOnMediaInfoListener_ != null) {
                                encoder.get().mOnMediaInfoListener_.onMediaTime((int) (videoBufferInfo.presentationTimeUs / 1000000));
                            }
                        }
                        videoEncoder.releaseOutputBuffer(outputBufferIndex, false);
                        outputBufferIndex = videoEncoder.dequeueOutputBuffer(videoBufferInfo, 0);
                    }
                }
            }
        }

        public void exit() {
            isExit = true;
        }
    }

    static class AudioEncoderThread extends Thread {
        private WeakReference<WLBaseMediaEncoder> encoder = null;
        private boolean isExit;
        private MediaCodec audioEncoder = null;
        private MediaCodec.BufferInfo bufferInfo = null;
        private MediaMuxer mediaMuxer = null;
        private int audioTrackIndex = -1;
        long pts;
        AudioEncoderThread(WeakReference<WLBaseMediaEncoder> encoder) {
            this.encoder = encoder;
            audioEncoder = encoder.get().mAudioEncoder_;
            bufferInfo = encoder.get().mAudioBufferInfo_;
            mediaMuxer = encoder.get().mMediaMuxer_;
            audioTrackIndex = -1;
        }

        @Override
        public void run() {
            super.run();
            pts = 0;
            audioTrackIndex = -1;
            isExit = false;
            audioEncoder.start();
            while (true) {
                if (isExit) {
                    audioEncoder.stop();
                    audioEncoder.release();
                    audioEncoder = null;
                    encoder.get().mAudioExit_ = true;
                    if (encoder.get().mVideoExit_) {
                        mediaMuxer.stop();
                        mediaMuxer.release();
                        mediaMuxer = null;
                        Log.i("LivePusherPlayer", "Audio 录制完成!!!");
                    }
                    break;
                }

                int outputBufferIndex = audioEncoder.dequeueOutputBuffer(bufferInfo, 0);
                if (outputBufferIndex == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                    if (mediaMuxer != null) {
                        audioTrackIndex = mediaMuxer.addTrack(audioEncoder.getOutputFormat());
                        if (encoder.get().mVideoEncoderThread_.videoTrackIndex != -1) {
                            mediaMuxer.start();
                            encoder.get().mEncoderStart_ = true;
                        }
                    }
                } else {
                    while (outputBufferIndex >= 0) {
                        if (encoder.get().mEncoderStart_) {
                            ByteBuffer outputBuffer = audioEncoder.getOutputBuffers()[outputBufferIndex];
                            outputBuffer.position(bufferInfo.offset);
                            outputBuffer.limit(bufferInfo.offset + bufferInfo.size);
                            if (pts == 0) {
                                pts = bufferInfo.presentationTimeUs;
                            }
                            bufferInfo.presentationTimeUs = bufferInfo.presentationTimeUs - pts;
                            mediaMuxer.writeSampleData(audioTrackIndex, outputBuffer, bufferInfo);
                        }
                        audioEncoder.releaseOutputBuffer(outputBufferIndex, false);
                        outputBufferIndex = audioEncoder.dequeueOutputBuffer(bufferInfo, 0);
                    }
                }
            }
        }

        public void exit() {
            isExit = true;
        }
    }

    private long getAudioPts(int size, int sampleRate) {
        mAudioPts_ += (long) (1.0 * size / (sampleRate * 2 * 2) * 1000000.0);
        return mAudioPts_;
    }
}
