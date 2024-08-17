package com.jiaquan.livepusher.encodec;

import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.media.MediaMuxer;
import android.opengl.EGLContext;
import android.util.Log;
import android.view.Surface;

import com.jiaquan.livepusher.egl.EglHelper;
import com.jiaquan.livepusher.egl.WLEGLSurfaceView;

import java.io.IOException;
import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;

public abstract class WLBaseMediaEncoder {
    public final static int RENDERMODE_WHEN_DIRTY = 0;
    public final static int RENDERMODE_CONTINUOUSLY = 1;
    private Surface mSurface_ = null;
    private EGLContext mEglContext_ = null;
    private int mEncodeWidth_ = -1;
    private int mEncodeHeight_ = -1;
    private MediaCodec mVideoEncoder_ = null;
    private MediaFormat mVideoFormat_ = null;
    private MediaCodec.BufferInfo mVideoBufferInfo_ = null;
    private MediaCodec mAudioEncoder_ = null;
    private MediaFormat mAudioFormat_ = null;
    private MediaCodec.BufferInfo mAudioBufferInfo_ = null;
    private long mAudioEncPts_ = 0;
    private int mSampleRate_ = 0;
    private MediaMuxer mMediaMuxer_ = null;
    private boolean mMuxerStart_;
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

    public WLBaseMediaEncoder() {

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

    public void initMediaEncoder(String savePath, int encodeWidth, int encodeHeight, int sampleRate, int channelCount) {
        try {
            _initVideoEncoder(MediaFormat.MIMETYPE_VIDEO_AVC, encodeWidth, encodeHeight);
            _initAudioEncoder(MediaFormat.MIMETYPE_AUDIO_AAC, sampleRate, channelCount);
            mMediaMuxer_ = new MediaMuxer(savePath, MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public void startRecord() {
        if (mSurface_ != null) {
            mAudioExit_ = false;
            mVideoExit_ = false;
            mMuxerStart_ = false;
            mAudioEncPts_ = 0;
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
            mWleglMediaThread_.exit();
            mVideoEncoderThread_ = null;
            mAudioEncoderThread_ = null;
            mWleglMediaThread_ = null;
        }
    }

    public void putPCMData(byte[] buffer, int size) {
        if ((mAudioEncoderThread_ != null) && !mAudioEncoderThread_.mIsExit_ && (buffer != null) && (size > 0)) {
            int inputBufferIndex = mAudioEncoder_.dequeueInputBuffer(0);
            if (inputBufferIndex >= 0) {
                ByteBuffer byteBuffer = mAudioEncoder_.getInputBuffers()[inputBufferIndex];
                byteBuffer.clear();
                byteBuffer.put(buffer);
                /**
                 * 这里的size是pcm数据的大小，这里的pts是根据pcm数据的大小和采样率计算出来的时间戳
                 * 但是给到音频编码器并不一定需要这个时间戳，因为音频编码器会自己计算时间戳。
                 */
                long pts = _getAudioPts(size, mSampleRate_);
                mAudioEncoder_.queueInputBuffer(inputBufferIndex, 0, size, pts, 0);
            }
        }
    }

    private void _initVideoEncoder(String mimeType, int encodeWidth, int encodeHeight) {
        Log.i("LivePusherPlayer", "initVideoEncoder: encodeWidth: " + encodeWidth + ", encodeHeight: " + encodeHeight);
        mEncodeWidth_ = encodeWidth;
        mEncodeHeight_ = encodeHeight;
        try {
            mVideoBufferInfo_ = new MediaCodec.BufferInfo();
            mVideoFormat_ = MediaFormat.createVideoFormat(mimeType, encodeWidth, encodeHeight);
            mVideoFormat_.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface);
            /**
             * 默认是VBR码率模式，可变码率模式，即使设置了目标码率，实际上编码出来的码率是不固定的。只是说会尽量控制在目标码率的范围内。
             * 如果想要固定码率，需要设置KEY_BITRATE_MODE为BITRATE_MODE_CBR，固定码率模式。那么这里的KEY_BIT_RATE就是固定的码率。
             */
            mVideoFormat_.setInteger(MediaFormat.KEY_BIT_RATE, encodeWidth * encodeHeight * 4);
            mVideoFormat_.setInteger(MediaFormat.KEY_BITRATE_MODE, MediaCodecInfo.EncoderCapabilities.BITRATE_MODE_VBR);
            /**
             * 设置KEY_FRAME_RATE和KEY_I_FRAME_INTERVAL，可以控制视频的关键帧间隔，这里表示30帧一个关键帧
             * 单独设置是无法控制的，需要同时设置。
             * 另外这里设置的是30帧的帧率，实际上是无效的，是由喂给的实际图像数据决定的。
             */
            mVideoFormat_.setInteger(MediaFormat.KEY_FRAME_RATE, 30);
            mVideoFormat_.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, 1);
            mVideoEncoder_ = MediaCodec.createEncoderByType(mimeType);
            Log.i("LivePusherPlayer", "initVideoEncoder: mVideoFormat_: " + mVideoFormat_);
            mVideoEncoder_.configure(mVideoFormat_, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
            mSurface_ = mVideoEncoder_.createInputSurface();//获取硬件编码器的输入surface
        } catch (IOException e) {
            e.printStackTrace();
            mVideoEncoder_ = null;
            mVideoFormat_ = null;
            mVideoBufferInfo_ = null;
        }
    }

    private void _initAudioEncoder(String mimeType, int sampleRate, int channelCount) {
        try {
            mSampleRate_ = sampleRate;
            mAudioBufferInfo_ = new MediaCodec.BufferInfo();
            mAudioFormat_ = MediaFormat.createAudioFormat(mimeType, sampleRate, channelCount);
            mAudioFormat_.setInteger(MediaFormat.KEY_BIT_RATE, 96000);//AAC-HE-AACv2 32kbps-96kbps  设置的是96kbps码率
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

    static class WLEGLMediaThread extends Thread {
        private WeakReference<WLBaseMediaEncoder> mBaseMediaEncoder_;
        private EglHelper mEglHelper_ = null;
        private Object mObject_;
        private boolean mIsExit_ = false;
        private boolean mIsCreate_ = false;
        private boolean mIsChange_ = false;
        private boolean mIsStart_ = false;

        /**
         * 在Java中，私有成员变量的访问权限控制是基于类而不是基于实例的。
         * 这意味着，同一个类的不同实例可以互相访问彼此的私有成员。
         * 因此，在内部类（包括匿名内部类和线程）中可以访问外围类的私有成员。
         * WLEGLMediaThread是通过持有WLBaseMediaEncoder对象的弱引用来间接访问它的。
         * 这是可以的，因为WLEGLMediaThread可能被视为WLBaseMediaEncoder的内部使用类，
         * 并且Java的访问控制允许这样的访问操作。
         */
        public WLEGLMediaThread(WeakReference<WLBaseMediaEncoder> encoder) {
            mBaseMediaEncoder_ = encoder;
        }

        @Override
        public void run() {
            super.run();
            mIsExit_ = false;
            mIsStart_ = false;
            mObject_ = new Object();
            mEglHelper_ = new EglHelper();
            mEglHelper_.initEgl(mBaseMediaEncoder_.get().mSurface_, mBaseMediaEncoder_.get().mEglContext_);
            while (true) {
                if (mIsExit_) {
                    release();
                    break;
                }
                if (mIsStart_) {
                    if (mBaseMediaEncoder_.get().mRenderMode_ == RENDERMODE_WHEN_DIRTY) {
                        synchronized (mObject_) {
                            try {
                                mObject_.wait();
                            } catch (InterruptedException e) {
                                e.printStackTrace();
                            }
                        }
                    } else if (mBaseMediaEncoder_.get().mRenderMode_ == RENDERMODE_CONTINUOUSLY) {
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
                onChange(mBaseMediaEncoder_.get().mEncodeWidth_, mBaseMediaEncoder_.get().mEncodeHeight_);
                onDraw();
                mIsStart_ = true;
            }
        }

        private void onCreate() {
            if (mIsCreate_ && (mBaseMediaEncoder_.get().mWlglRender_ != null)) {
                mIsCreate_ = false;
                mBaseMediaEncoder_.get().mWlglRender_.onSurfaceCreated();
            }
        }

        private void onChange(int width, int height) {
            if (mIsChange_ && (mBaseMediaEncoder_.get().mWlglRender_ != null)) {
                mIsChange_ = false;
                mBaseMediaEncoder_.get().mWlglRender_.onSurfaceChanged(width, height);
            }
        }

        private void onDraw() {
            if ((mBaseMediaEncoder_.get().mWlglRender_ != null) && (mEglHelper_ != null) && mIsStart_) {
                mBaseMediaEncoder_.get().mWlglRender_.onDrawFrame();
                mEglHelper_.swapBuffers();
            }
        }

        public void exit() {
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
            if (mBaseMediaEncoder_.get().mWlglRender_ != null) {
                mBaseMediaEncoder_.get().mWlglRender_.onSurfaceDestroy();
                mBaseMediaEncoder_.get().mWlglRender_ = null;
            }
            if (mEglHelper_ != null) {
                mEglHelper_.destroyEgl();
                mEglHelper_ = null;
                mObject_ = null;
            }
            mBaseMediaEncoder_ = null;
        }
    }

    static class VideoEncoderThread extends Thread {
        private WeakReference<WLBaseMediaEncoder> mBaseMediaEncoder_ = null;
        private boolean mIsExit_;
        private MediaCodec videoEncoder = null;
        private MediaCodec.BufferInfo videoBufferInfo = null;
        private MediaMuxer mediaMuxer = null;
        private long pts;
        private int videoTrackIndex = -1;

        VideoEncoderThread(WeakReference<WLBaseMediaEncoder> encoder) {
            mBaseMediaEncoder_ = encoder;
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
            mIsExit_ = false;
            videoEncoder.start();
            while (true) {
                if (mIsExit_) {
                    videoEncoder.stop();
                    videoEncoder.release();
                    videoEncoder = null;
                    mBaseMediaEncoder_.get().mVideoExit_ = true;
                    if (mBaseMediaEncoder_.get().mAudioExit_) {
                        mediaMuxer.stop();
                        mediaMuxer.release();
                        mediaMuxer = null;
                        mBaseMediaEncoder_.get().mAudioExit_ = false;
                        Log.i("LivePusherPlayer", "Video 录制完成!!!");
                    }
                    break;
                }
                int outputBufferIndex = videoEncoder.dequeueOutputBuffer(videoBufferInfo, 0);
                if (outputBufferIndex == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                    videoTrackIndex = mediaMuxer.addTrack(videoEncoder.getOutputFormat());
                    if (mBaseMediaEncoder_.get().mAudioEncoderThread_.audioTrackIndex != -1) {
                        mediaMuxer.start();
                        mBaseMediaEncoder_.get().mMuxerStart_ = true;
                    }
                } else {
                    while (outputBufferIndex >= 0) {//循环获取数据写入Muxer
                        if (mBaseMediaEncoder_.get().mMuxerStart_) {
                            ByteBuffer outputBuffer = videoEncoder.getOutputBuffers()[outputBufferIndex];
                            outputBuffer.position(videoBufferInfo.offset);
                            outputBuffer.limit(videoBufferInfo.offset + videoBufferInfo.size);
                            /**
                             * 校准时间戳以确保其从零开始并连续增长，以提高媒体文件的兼容性和播放质量。
                             * 这里避免那种负数的时间戳，因为有些播放器不支持负数的时间戳。
                             */
                            if (pts == 0) {
                                pts = videoBufferInfo.presentationTimeUs;
                            }
                            videoBufferInfo.presentationTimeUs = videoBufferInfo.presentationTimeUs - pts;
                            mediaMuxer.writeSampleData(videoTrackIndex, outputBuffer, videoBufferInfo);
                            if (mBaseMediaEncoder_.get().mOnMediaInfoListener_ != null) {
                                mBaseMediaEncoder_.get().mOnMediaInfoListener_.onMediaTime((int) (videoBufferInfo.presentationTimeUs / 1000000));
                            }
                        }
                        videoEncoder.releaseOutputBuffer(outputBufferIndex, false);
                        outputBufferIndex = videoEncoder.dequeueOutputBuffer(videoBufferInfo, 0);
                    }
                }
            }
        }

        public void exit() {
            mIsExit_ = true;
        }
    }

    static class AudioEncoderThread extends Thread {
        private WeakReference<WLBaseMediaEncoder> mBaseMediaEncoder_ = null;
        private boolean mIsExit_ = false;
        private MediaCodec audioEncoder = null;
        private MediaCodec.BufferInfo audioBufferInfo = null;
        private MediaMuxer mediaMuxer = null;
        private int audioTrackIndex = -1;
        long pts = 0;

        AudioEncoderThread(WeakReference<WLBaseMediaEncoder> encoder) {
            mBaseMediaEncoder_ = encoder;
            audioEncoder = encoder.get().mAudioEncoder_;
            audioBufferInfo = encoder.get().mAudioBufferInfo_;
            mediaMuxer = encoder.get().mMediaMuxer_;
            audioTrackIndex = -1;
        }

        @Override
        public void run() {
            super.run();
            pts = 0;
            audioTrackIndex = -1;
            mIsExit_ = false;
            audioEncoder.start();
            while (true) {
                if (mIsExit_) {
                    audioEncoder.stop();
                    audioEncoder.release();
                    audioEncoder = null;
                    mBaseMediaEncoder_.get().mAudioExit_ = true;
                    if (mBaseMediaEncoder_.get().mVideoExit_) {
                        mediaMuxer.stop();
                        mediaMuxer.release();
                        mediaMuxer = null;
                        mBaseMediaEncoder_.get().mVideoExit_ = false;
                        Log.i("LivePusherPlayer", "Audio录制完成!!!");
                    }
                    break;
                }
                int outputBufferIndex = audioEncoder.dequeueOutputBuffer(audioBufferInfo, 0);
                if (outputBufferIndex == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                    if (mediaMuxer != null) {
                        audioTrackIndex = mediaMuxer.addTrack(audioEncoder.getOutputFormat());
                        if (mBaseMediaEncoder_.get().mVideoEncoderThread_.videoTrackIndex != -1) {
                            mediaMuxer.start();
                            mBaseMediaEncoder_.get().mMuxerStart_ = true;
                        }
                    }
                } else {
                    while (outputBufferIndex >= 0) {
                        if (mBaseMediaEncoder_.get().mMuxerStart_) {
                            ByteBuffer outputBuffer = audioEncoder.getOutputBuffers()[outputBufferIndex];
                            outputBuffer.position(audioBufferInfo.offset);
                            outputBuffer.limit(audioBufferInfo.offset + audioBufferInfo.size);
                            if (pts == 0) {
                                pts = audioBufferInfo.presentationTimeUs;
                            }
                            audioBufferInfo.presentationTimeUs = audioBufferInfo.presentationTimeUs - pts;
                            mediaMuxer.writeSampleData(audioTrackIndex, outputBuffer, audioBufferInfo);
                        }
                        audioEncoder.releaseOutputBuffer(outputBufferIndex, false);
                        outputBufferIndex = audioEncoder.dequeueOutputBuffer(audioBufferInfo, 0);
                    }
                }
            }
        }

        public void exit() {
            mIsExit_ = true;
        }
    }

    /**
     * 根据音频pcm数据量计算当前时间戳
     */
    private long _getAudioPts(int pcmDataSize, int sampleRate) {
        mAudioEncPts_ += (long) (1.0 * pcmDataSize / (sampleRate * 2 * 2) * 1000000.0);
        return mAudioEncPts_;
    }
}
