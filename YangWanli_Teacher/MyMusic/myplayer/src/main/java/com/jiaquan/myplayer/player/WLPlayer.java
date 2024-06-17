package com.jiaquan.myplayer.player;

import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.media.MediaFormat;
import android.os.Build;
import android.os.Handler;
import android.os.HandlerThread;
import android.text.TextUtils;
import android.view.Surface;

import androidx.annotation.RequiresApi;

import com.jiaquan.myplayer.listener.OnCompleteListener;
import com.jiaquan.myplayer.listener.OnErrorListener;
import com.jiaquan.myplayer.listener.OnLoadListener;
import com.jiaquan.myplayer.listener.OnPauseResumeListener;
import com.jiaquan.myplayer.listener.OnPcmInfoListener;
import com.jiaquan.myplayer.listener.OnPreparedListener;
import com.jiaquan.myplayer.listener.OnRecordTimeListener;
import com.jiaquan.myplayer.listener.OnTimeInfoListener;
import com.jiaquan.myplayer.listener.OnVolumeDBListener;
import com.jiaquan.myplayer.log.MyLog;
import com.jiaquan.myplayer.muteenum.ChannelTypeEnum;
import com.jiaquan.myplayer.opengl.WLGLSurfaceView;
import com.jiaquan.myplayer.opengl.WLRender;
import com.jiaquan.myplayer.util.CalledByNative;
import com.jiaquan.myplayer.util.TimeInfoBean;
import com.jiaquan.myplayer.util.WLVideoSupportUtil;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;

public class WLPlayer {
    static {
        System.loadLibrary("native-lib");
        System.loadLibrary("avcodec");
        System.loadLibrary("avdevice");
        System.loadLibrary("avfilter");
        System.loadLibrary("avformat");
        System.loadLibrary("avutil");
        System.loadLibrary("postproc");
        System.loadLibrary("swresample");
        System.loadLibrary("swscale");
    }

    private WLGLSurfaceView mWlglSurfaceView_ = null;
    private Surface mSurface_ = null;
    private MediaCodec mVDecMediaCodec_ = null;
    private MediaCodec.BufferInfo mVBufferInfo_ = null;
    private static TimeInfoBean mTimeInfoBean_ = null;
    private long mStartMs_ = 0;//记录每次硬解解码前的系统时间
    public int mFrameCount_ = 0;//记录硬解播放的总帧数
    public long mTotalTime_ = 0;//记录硬解耗时
    private int mVolumePercent_ = 100;
    private ChannelTypeEnum mChannelType_ = ChannelTypeEnum.MUTE_CENTER;
    private float mSpeed_ = 1.0f;
    private float mPitch_ = 1.0f;
    private final Object mVLock_ = new Object(); // 用于同步的锁对象
    private Handler mMultiVideoHandler_ = null;
    private HandlerThread mMultiVideoHandlerThread_ = null;

    public void setWlglSurfaceView(WLGLSurfaceView wlglSurfaceView) {
        mWlglSurfaceView_ = wlglSurfaceView;
        wlglSurfaceView.getWlRender().setOnSurfaceCreateListener(new WLRender.OnSurfaceCreateListener() {
            @Override
            public void onSurfaceCreate(Surface s) {
                if (mSurface_ == null) {
                    mSurface_ = s;//监听获取到的surface,用于配置给硬件解码器
                    MyLog.i("onSurfaceCreate get surface");
                }
            }
        });
    }

    private OnLoadListener mOnLoadListener_ = null;
    public void setOnLoadListener(OnLoadListener onLoadListener) {
        mOnLoadListener_ = onLoadListener;
    }

    private OnPreparedListener mOnPreparedListener_ = null;
    public void setOnPreparedListener(OnPreparedListener onPreparedListener) {
        mOnPreparedListener_ = onPreparedListener;
    }

    private OnPauseResumeListener mOnPauseResumeListener_ = null;
    public void setOnPauseResumeListener(OnPauseResumeListener onPauseResumeListener) {
        mOnPauseResumeListener_ = onPauseResumeListener;
    }

    private OnTimeInfoListener mOnTimeInfoListener_ = null;
    public void setOnTimeInfoListener(OnTimeInfoListener onTimeInfoListener) {
        mOnTimeInfoListener_ = onTimeInfoListener;
    }

    private OnPcmInfoListener mOnPcmInfoListener_ = null;
    public void setOnPcmInfoListener(OnPcmInfoListener onPcmInfoListener) {
        mOnPcmInfoListener_ = onPcmInfoListener;
    }

    private OnVolumeDBListener mOnVolumeDBListener_ = null;
    public void setOnVolumeDBListener(OnVolumeDBListener onVolumeDBListener) {
        mOnVolumeDBListener_ = onVolumeDBListener;
    }

    private OnCompleteListener mOnCompleteListener_ = null;
    public void setOnCompleteListener(OnCompleteListener onCompleteListener) {
        mOnCompleteListener_ = onCompleteListener;
    }

    private OnErrorListener mOnErrorListener_ = null;
    public void setOnErrorListener(OnErrorListener onErrorListener) {
        mOnErrorListener_ = onErrorListener;
    }

    private OnRecordTimeListener mOnRecordTimeListener_ = null;
    public void setOnRecordTimeListener(OnRecordTimeListener onRecordTimeListener) {
        mOnRecordTimeListener_ = onRecordTimeListener;
    }

    @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
    private void _getSupportedCodec() {
        MyLog.i("print getSupportCodec");
        MediaCodecList list = new MediaCodecList(MediaCodecList.REGULAR_CODECS);
        MediaCodecInfo[] codecs = list.getCodecInfos();
        MyLog.i("Decoders:");
        for (MediaCodecInfo codec : codecs) {//遍历所有的编解码器类型
            if (!codec.isEncoder()) {
                //基本就是一个name对应一个type，但是不同的name可能对应的是同一个type
                String[] types = codec.getSupportedTypes();
                for (String type : types) {
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                        MyLog.i("Name: " + codec.getName() + ", Type: " + type + ", isHardwareAccelerated: " + codec.isHardwareAccelerated());
                    }
                }
            }
        }

        MyLog.i("Encoders:");
        for (MediaCodecInfo codec : codecs) {
            if (codec.isEncoder()) {
                String[] types = codec.getSupportedTypes();
                for (String type : types) {
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                        MyLog.i("Name: " + codec.getName() + ", Type: " + type + ", isHardwareAccelerated: " + codec.isHardwareAccelerated());
                    }
                }
            }
        }
    }

    public WLPlayer() {
        //仅用于测试：打印获取该机器支持的编解码器类型
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            _getSupportedCodec();
        }

        mMultiVideoHandlerThread_ = new HandlerThread("MultiVideoHandlerThread");
        mMultiVideoHandlerThread_.start();
        mMultiVideoHandler_ = new Handler(mMultiVideoHandlerThread_.getLooper());
    }

    /**
     * 准备播放源
     */
    public void prepare(String playSource) {
        MyLog.i("prepare in source: " + playSource);
        if (TextUtils.isEmpty(playSource)) {
            MyLog.i("source must not be empty");
            return;
        }

        stop();//先停止播放

        if (mMultiVideoHandler_ != null) {
            mMultiVideoHandler_.post(new Runnable() {
                @Override
                public void run() {
                    MyLog.i("prepare in thread start");
                    _nativePrepare(playSource);
                    MyLog.i("prepare in thread end");
                }
            });
        }
        MyLog.i("prepare out");
    }

    /**
     * 开始播放
     */
    public void start() {
        if (mMultiVideoHandler_ != null) {
            mMultiVideoHandler_.post(new Runnable() {
                @Override
                public void run() {
                    setVolume(mVolumePercent_);
                    setChannelType(mChannelType_);
                    setPitch(mPitch_);
                    setSpeed(mSpeed_);

                    _nativeStart();
                }
            });
        }
    }

    /**
     * 暂停播放
     */
    public void pause() {
        _nativePause();
        if (mOnPauseResumeListener_ != null) {
            mOnPauseResumeListener_.onPause(true);
        }
    }

    /**
     * 恢复播放
     */
    public void resume() {
        _nativeResume();
        if (mOnPauseResumeListener_ != null) {
            mOnPauseResumeListener_.onPause(false);
        }
    }

    /**
     * 停止播放
     */
    public void stop() {
        MyLog.i("stop in mTotalTime_: " + mTotalTime_ + " mFrameCount_: " + mFrameCount_);
        if ((mTotalTime_ > 0) && (mFrameCount_ > 0)) {
            MyLog.i("All the Frames: " + mFrameCount_ + " Average decode time per frame: " + (mTotalTime_ / mFrameCount_) + "ms");
        }
        if (mMultiVideoHandler_ != null) {
            mMultiVideoHandler_.post(new Runnable() {
                @Override
                public void run() {
                    MyLog.i("stop in thread start");
                    stopAudioRecord();
                    _nativeStop();
                    _releaseVMediaCodec();

                    mTimeInfoBean_ = null;
                    mTotalTime_ = 0;//记录硬解耗时
                    mFrameCount_ = 0;//记录硬解播放的总帧数
                    mStartMs_ = 0;//记录每次硬解解码前的系统时间
                    MyLog.i("stop in thread end");
                }
            });
        }
        MyLog.i("stop out");
    }

    /**
     * seek到指定位置
     * @param secds 指定位置
     */
    public void seek(int secds) {
        _nativeSeek(secds);
    }

    /**
     * 播放下一个
     * @param url 下一个视频地址
     */
    public void playNext(String url) {
        MyLog.i("playNext in url: " + url);
        prepare(url);
    }

    /**
     * 获取视频总时长
     * @return 视频总时长
     */
    public int getDuration() {
        return _nativeDuration();
    }

    /**
     * 设置音量
     * @param percent 音量百分比
     */
    public void setVolume(int percent) {
        if ((percent >= 0) && (percent <= 100)) {
            mVolumePercent_ = percent;
            _nativeVolume(percent);
        }
    }

     /**
     * 获取音量
     * @return 音量百分比
     */
    public int getVolumePercent() {
        return mVolumePercent_;
    }

    /**
     * 设置声道
     * @param channelType 声道类型
     */
    public void setChannelType(ChannelTypeEnum channelType) {
        mChannelType_ = channelType;
        _nativeChannelType(mChannelType_.getValue());
    }

    /**
     * 获取音调值
     * @param p 音调值
     */
    public void setPitch(float p) {
        mPitch_ = p;
        _nativePitch(mPitch_);
    }

    /**
     * 获取播放速度
     * @param s 播放速度
     */
    public void setSpeed(float s) {
        mSpeed_ = s;
        _nativeSpeed(mSpeed_);
    }

    /**
     * 开始剪切音频播放
     * @param startTime 开始时间
     * @param endTime 结束时间
     * @param showPcm 是否显示pcm数据
     */
    public void cutAudioPlay(int startTime, int endTime, boolean showPcm) {
        if (_nativeCutAudioPlay(startTime, endTime, showPcm)) {//先seek到指定位置
            start();
        } else {
            stop();
            onCallError(2001, "cutAudioPlay params is wrong!");
        }
    }

    private void _releaseVMediaCodec() {
        synchronized (mVLock_) {
            MyLog.i("releaseVMediaCodec in: " + mVDecMediaCodec_);
            if (mVDecMediaCodec_ != null) {
                try {
                    mVDecMediaCodec_.flush();
                    mVDecMediaCodec_.stop();
                    mVDecMediaCodec_.release();
                } catch (Exception e) {
                    e.printStackTrace();
                }
                mVDecMediaCodec_ = null;
                mVBufferInfo_ = null;
                MyLog.i("releaseVMediaCodec out");
            }
        }
    }

    private void _printBytesInLines(byte[] bytes, int len) {
        StringBuilder stringBuilder = new StringBuilder();
        for (int i = 0; i < len; i++) {
            stringBuilder.append(String.format("%02X ", bytes[i]));
            if ((i + 1) % 16 == 0) {
                MyLog.i(stringBuilder.toString());
                stringBuilder.setLength(0);
            }
        }
        if (stringBuilder.length() > 0) {
            MyLog.i(stringBuilder.toString());
        }
    }

    @CalledByNative
    private void onCallLoad(boolean load) {
        if (mOnLoadListener_ != null) {
            mOnLoadListener_.onLoad(load);
        }
    }

    @CalledByNative
    private void onCallPrepared() {
        if (mOnPreparedListener_ != null) {
            mOnPreparedListener_.onPrepared();
        }
    }

    @CalledByNative
    private void onCallTimeInfo(int currentTime, int totalTime) {
        if (mOnTimeInfoListener_ != null) {
            if (mTimeInfoBean_ == null) {
                mTimeInfoBean_ = new TimeInfoBean();
            }
            mTimeInfoBean_.setCurrentTime(currentTime);
            mTimeInfoBean_.setTotalTime(totalTime);
            mOnTimeInfoListener_.onTimeInfo(mTimeInfoBean_);
        }
    }

    @CalledByNative
    private void onCallPcmRate(int samplerate, int bit, int channels) {
        if (mOnPcmInfoListener_ != null) {
            mOnPcmInfoListener_.onPcmRate(samplerate, bit, channels);
        }
    }

    @CalledByNative
    private void onCallPcmInfo(byte[] buffer, int bufferSize) {
        if (mOnPcmInfoListener_ != null) {
            mOnPcmInfoListener_.onPcmInfo(buffer, bufferSize);
        }
    }

    @CalledByNative
    private void onCallVolumeDB(int db) {
        if (mOnVolumeDBListener_ != null) {
            mOnVolumeDBListener_.onDBValue(db);
        }
    }

    @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
    @CalledByNative
    private boolean onCallIsSupportMediaCodec(String codecTag) {
        String codecName = WLVideoSupportUtil.getHardwareDecoderName(codecTag);
        if (TextUtils.isEmpty(codecName)) {
            MyLog.i("onCallIsSupportMediaCodec is not support");
            return false;
        }
        return true;
    }

    @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
    @CalledByNative
    private void onCallInitMediaCodec(String codecTag, int width, int height, byte[] csd) {
        synchronized (mVLock_) {
            if (mSurface_ != null) {
                try {
                    mWlglSurfaceView_.getWlRender().setRenderType(WLRender.RENDER_MEDIACODEC);
                    mWlglSurfaceView_.getWlRender().setVideoSize(width, height);

                    String mimeType = WLVideoSupportUtil.findVideoCodecType(codecTag);
                    MyLog.i("onCallinitMediaCodec mime is " + mimeType + " width is " + width + " height is " + height);
                    MediaFormat videoFormat = MediaFormat.createVideoFormat(mimeType, width, height);
                    videoFormat.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, width * height);
                    /**
                     * 这里三个字段都是设置为ffmpeg提取的extradata数据，目前硬件解码是没问题的，
                     * 理论上是需要分别提取SPS和PPS数据填充设置,H265需要设置VPS，SPS，PPS三个字段.
                     * 应该MediaCodec针对直接传递的extradata数据在内部进行了提取VPS,SPS,PPS,比较强大
                     */
//                    MyLog.i("java onCallinitMediaCodec csd size: " + csd.length);
//                    _printBytesInLines(csd, csd.length);
                    videoFormat.setByteBuffer("csd-0", ByteBuffer.wrap(csd));
                    videoFormat.setByteBuffer("csd-1", ByteBuffer.wrap(csd));
                    if (mimeType.equals("video/hevc")) {
                        videoFormat.setByteBuffer("csd-2", ByteBuffer.wrap(csd));
                    }
                    MyLog.i(videoFormat.toString());
                    mVBufferInfo_ = new MediaCodec.BufferInfo();
                    mVDecMediaCodec_ = MediaCodec.createByCodecName(WLVideoSupportUtil.getHardwareDecoderName(codecTag));
                    mVDecMediaCodec_.configure(videoFormat, mSurface_, null, 0);
                    mVDecMediaCodec_.start();
                } catch (Exception e) {
                    e.printStackTrace();
                }
                mStartMs_ = System.currentTimeMillis();
                MyLog.i("onCallinitMediaCodec end");
            } else {
                if (mOnErrorListener_ != null) {
                    mOnErrorListener_.onError(2001, "surface is null");
                }
            }
        }
    }

   @CalledByNative
    private void onCallDecodeVPacket(int datasize, byte[] data) {
        /**
         *这里的码流数据，底层使用了ffmpeg进行了过滤，转换为AnnexB模式，在如果是I帧，会在I帧前补充添加带有startcode的VPS,SPS,PPS，
         * 然后再跟上实际的startcode + I帧图像数据。如果是非I帧，会直接在前面将AVCC字段转换为00 00 00 01的startcode + 实际的图像数据
         */
//        MyLog.i("data size: " + data.length);
//        _printBytesInLines(data, 1000);
         synchronized (mVLock_) {
             if ((mSurface_ != null) && (datasize > 0) && (data != null) && (mVDecMediaCodec_ != null)) {
                 try {
                     int inputBufferIndex = mVDecMediaCodec_.dequeueInputBuffer(10);
                     if (inputBufferIndex >= 0) {
                         ByteBuffer byteBuffer = mVDecMediaCodec_.getInputBuffers()[inputBufferIndex];
                         byteBuffer.clear();
                         byteBuffer.put(data);
                         mVDecMediaCodec_.queueInputBuffer(inputBufferIndex, 0, datasize, 0, 0);//丢给mediaCodec解码输入队列
                     }
                     int outputBufferIndex = mVDecMediaCodec_.dequeueOutputBuffer(mVBufferInfo_, 10);
                     while (outputBufferIndex >= 0) {//循环从硬解解码器的输出队列中获取解码数据进行渲染
                         long decodeTime = System.currentTimeMillis() - mStartMs_;
                         mStartMs_ = System.currentTimeMillis();
                         mFrameCount_++;
                         mTotalTime_ += decodeTime;
                         mVDecMediaCodec_.releaseOutputBuffer(outputBufferIndex, true);
                         outputBufferIndex = mVDecMediaCodec_.dequeueOutputBuffer(mVBufferInfo_, 10);
                     }
                 } catch (Exception e) {
                     e.printStackTrace();
                 }
             }
         }
    }

    @CalledByNative
    private void onCallRenderYUV(int width, int height, byte[] y, byte[] u, byte[] v) {
        if (mWlglSurfaceView_ != null) {
            mWlglSurfaceView_.getWlRender().setRenderType(WLRender.RENDER_YUV);
            mWlglSurfaceView_.setYUVData(width, height, y, u, v);
        }
    }

    @CalledByNative
    private void onCallComplete() {
        MyLog.i("onCallComplete start");
        stop();
        if (mOnCompleteListener_ != null) {
            mOnCompleteListener_.onComplete();
        }
        MyLog.i("onCallComplete end");
    }

    @CalledByNative
    private void onCallError(int code, String msg) {
        stop();
        if (mOnErrorListener_ != null) {
            mOnErrorListener_.onError(code, msg);
        }
    }

    //录音操作
    private MediaFormat mAEncMediaFormat_ = null;
    private MediaCodec mAEncMediaCodec_ = null;
    private boolean mIsInitAMediaCodec_ = false;
    private FileOutputStream mFileOutputStream_ = null;
    private MediaCodec.BufferInfo mABufferInfo_ = null;
    private byte[] mOutByteBuffer_ = new byte[4096 * 3];
    private double mRecordTime_ = 0;
    private int mAudioSamplerate_ = 0;
    private final Object mALock_ = new Object(); // 用于同步的锁对象
    /**
     * 开始录音
     * @param outfile 录音文件
     */
    public void startAudioRecord(File outfile) {
        if (!mIsInitAMediaCodec_) {
            mAudioSamplerate_ = _nativeSamplerate();
            if (mAudioSamplerate_ > 0) {
                try {
                    mFileOutputStream_ = new FileOutputStream(outfile);
                } catch (FileNotFoundException e) {
                    MyLog.e("create file output stream failed");
                    throw new RuntimeException(e);
                }
                mRecordTime_ = 0;

                _initAudioMediaCodec(mAudioSamplerate_);
                mIsInitAMediaCodec_ = true;
                _nativeStartstopRecord(true);
                MyLog.i("开始录音....");
            } else {
                MyLog.e("startAudioRecord failed");
            }
        }
    }

    /**
     * 暂停录音
     */
    public void pauseAudioRecord() {
        _nativeStartstopRecord(false);
        MyLog.i("暂停录音....");
    }

    /**
     * 恢复录音
     */
    public void resumeAudioRecord() {
        _nativeStartstopRecord(true);
        MyLog.i("恢复录音....");
    }

    /**
     * 停止录音
     */
    public void stopAudioRecord() {
        if (mIsInitAMediaCodec_) {
            _nativeStartstopRecord(false);
            synchronized (mALock_) {
                _releaseAMediaCodec();
                try {
                    mFileOutputStream_.close();
                    mFileOutputStream_ = null;
                } catch (IOException e) {
                    throw new RuntimeException(e);
                }
                mRecordTime_ = 0;
                mAudioSamplerate_ = 0;
                mIsInitAMediaCodec_ = false;
            }
            MyLog.i("完成录音....");
        }
    }

    private void _initAudioMediaCodec(int samplerate) {
        mAEncMediaFormat_ = MediaFormat.createAudioFormat(MediaFormat.MIMETYPE_AUDIO_AAC, samplerate, 2);
        mAEncMediaFormat_.setInteger(MediaFormat.KEY_BIT_RATE, 96000);
        mAEncMediaFormat_.setInteger(MediaFormat.KEY_AAC_PROFILE, MediaCodecInfo.CodecProfileLevel.AACObjectLC);
        mAEncMediaFormat_.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, 4096);
        mABufferInfo_ = new MediaCodec.BufferInfo();
        try {
            mAEncMediaCodec_ = MediaCodec.createEncoderByType(MediaFormat.MIMETYPE_AUDIO_AAC);
        } catch (IOException e) {
            MyLog.e("create audio encoder failed");
            mAEncMediaCodec_ = null;
            throw new RuntimeException(e);
        }
        mAEncMediaCodec_.configure(mAEncMediaFormat_, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);//配置编码器
        mAEncMediaCodec_.start();
    }

    //为每个AAC码流包增加ADTS头
    private void _addADTSHeader(byte[] packet, int packetLen, int rateType) {
        int audioObjectType = 2; // AAC LC
        int profile = audioObjectType - 1;
        int freqIdx = rateType; // sample rate index
        int chanCfg = 2; // CPE

        // fill in ADTS data
        packet[0] = (byte) 0xFF;
        packet[1] = (byte) 0xF9;
        packet[2] = (byte) (((profile - 1) << 6) + (freqIdx << 2) + (chanCfg >> 2));
        packet[3] = (byte) (((chanCfg & 3) << 6) + (packetLen >> 11));
        packet[4] = (byte) ((packetLen & 0x7FF) >> 3);
        packet[5] = (byte) (((packetLen & 7) << 5) + 0x1F);
        packet[6] = (byte) 0xFC;
    }

    //获取ADTS对应的采样率值
    private int _getADTSSampleRateType(int samplerate) {
        int rateType = 4;
        switch (samplerate) {
            case 96000:
                rateType = 0;
                break;
            case 88200:
                rateType = 1;
                break;
            case 64000:
                rateType = 2;
                break;
            case 48000:
                rateType = 3;
                break;
            case 44100:
                rateType = 4;
                break;
            case 32000:
                rateType = 5;
                break;
            case 24000:
                rateType = 6;
                break;
            case 22050:
                rateType = 7;
                break;
            case 16000:
                rateType = 8;
                break;
            case 12000:
                rateType = 9;
                break;
            case 11025:
                rateType = 10;
                break;
            case 8000:
                rateType = 11;
                break;
            case 7350:
                rateType = 12;
                break;
        }
        return rateType;
    }

    //释放音频编码器
    private void _releaseAMediaCodec() {
        if (mAEncMediaCodec_ == null) {
            return;
        }
        mAEncMediaCodec_.stop();
        mAEncMediaCodec_.release();
        mAEncMediaCodec_ = null;
        mAEncMediaFormat_ = null;
        mABufferInfo_ = null;
    }

    @CalledByNative
    private void onCallPcmToAAC(byte[] pcmBuffer, int size) {
        synchronized (mALock_) {
            if ((pcmBuffer != null) && (size > 0) && (mAEncMediaCodec_ != null)) {
                mRecordTime_ += size * 1.0 / (mAudioSamplerate_ * 2 * 2);//计算当前包的时长，并累加
                if (mOnRecordTimeListener_ != null) {
                    mOnRecordTimeListener_.onAudioRecordTime((int) mRecordTime_);//回调当前录制时长,单位秒
                }

                int inputBufferIndex = mAEncMediaCodec_.dequeueInputBuffer(0);//获取到编码输入buffer的可用索引
                if (inputBufferIndex >= 0) {
                    ByteBuffer byteBuffer = mAEncMediaCodec_.getInputBuffers()[inputBufferIndex];//根据索引获取编码输入可用的空闲buffer
                    byteBuffer.clear();
                    byteBuffer.put(pcmBuffer);
                    mAEncMediaCodec_.queueInputBuffer(inputBufferIndex, 0, size, 0, 0);
                }

                int index = mAEncMediaCodec_.dequeueOutputBuffer(mABufferInfo_, 0);
                while (index >= 0) {
                    try {
                        ByteBuffer byteBuffer = mAEncMediaCodec_.getOutputBuffers()[index];
                        byteBuffer.position(mABufferInfo_.offset);
                        byteBuffer.limit(mABufferInfo_.offset + mABufferInfo_.size);

                        int packetSize = mABufferInfo_.size + 7;//AAC码流需要添加7字节的头
                        _addADTSHeader(mOutByteBuffer_, packetSize, _getADTSSampleRateType(mAudioSamplerate_));//mediacodec编码出来的aac码流没有aac头，增加AAC码流头

                        byteBuffer.get(mOutByteBuffer_, 7, mABufferInfo_.size);//将编码码流数据放入AAC码流头后面存放
                        byteBuffer.position(mABufferInfo_.offset);

                        mFileOutputStream_.write(mOutByteBuffer_, 0, packetSize);//将完整的一帧音频码流数据写入文件

                        mAEncMediaCodec_.releaseOutputBuffer(index, false);//取出码流数据后，释放这个buffer,返回给队列中循环使用
                        index = mAEncMediaCodec_.dequeueOutputBuffer(mABufferInfo_, 0);
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }
            }
        }
    }

    //native方法
    private native void _nativePrepare(String source);

    private native void _nativeStart();

    private native void _nativePause();

    private native void _nativeResume();

    private native void _nativeStop();

    private native void _nativeSeek(int secds);

    private native int _nativeDuration();

    private native void _nativeVolume(int percent);

    private native void _nativeChannelType(int mute);

    private native void _nativePitch(float pitch);

    private native void _nativeSpeed(float speed);

    private native int _nativeSamplerate();

    private native void _nativeStartstopRecord(boolean start);

    private native boolean _nativeCutAudioPlay(int start_time, int end_time, boolean showPcm);
}
