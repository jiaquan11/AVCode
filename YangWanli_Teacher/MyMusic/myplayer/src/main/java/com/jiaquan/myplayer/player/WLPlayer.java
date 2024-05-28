package com.jiaquan.myplayer.player;

import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.media.MediaFormat;
import android.os.Build;
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
import com.jiaquan.myplayer.muteenum.MuteEnum;
import com.jiaquan.myplayer.opengl.WLGLSurfaceView;
import com.jiaquan.myplayer.opengl.WLRender;
import com.jiaquan.myplayer.util.CalledByNative;
import com.jiaquan.myplayer.util.TimeInfoBean;
import com.jiaquan.myplayer.util.WLVideoSupportUtil;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;

public class WLPlayer {
    // 加载动态库
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

    private String mPlayPath_ = null;
    private boolean mPlayNext_ = false;
    private int mDuration_ = -1;
    private int mVolumePercent_ = 100;
    private MuteEnum mMuteEnum_ = MuteEnum.MUTE_CENTER;
    private float mSpeed_ = 1.0f;
    private float mPitch_ = 1.0f;
    private boolean mIsInitMediaCodec_ = false;

    private MediaFormat mMediaFormat_ = null;
    private MediaCodec mMediaCodec_ = null;
    private Surface mSurface_ = null;
    private MediaCodec.BufferInfo mInfo_ = null;
    public long mTotalTime_ = 0;//记录硬解耗时
    public int mFrameCount_ = 0;//记录硬解播放的总帧数
    private long mStartMs_ = 0;//记录每次硬解解码前的系统时间
    private static TimeInfoBean mTimeInfoBean_ = null;
    private WLGLSurfaceView mWlglSurfaceView_ = null;

    public void setWlglSurfaceView(WLGLSurfaceView wlglSurfaceView) {
        this.mWlglSurfaceView_ = wlglSurfaceView;
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

    private OnLoadListener onLoadListener = null;
    public void setOnLoadListener(OnLoadListener onLoadListener) {
        this.onLoadListener = onLoadListener;
    }

    private OnPreparedListener onPreparedListener = null;
    public void setOnPreparedListener(OnPreparedListener onPreparedListener) {
        this.onPreparedListener = onPreparedListener;
    }

    private OnPauseResumeListener onPauseResumeListener = null;
    public void setOnPauseResumeListener(OnPauseResumeListener onPauseResumeListener) {
        this.onPauseResumeListener = onPauseResumeListener;
    }

    private OnTimeInfoListener onTimeInfoListener = null;
    public void setOnTimeInfoListener(OnTimeInfoListener onTimeInfoListener) {
        this.onTimeInfoListener = onTimeInfoListener;
    }

    private OnCompleteListener onCompleteListener = null;
    public void setOnCompleteListener(OnCompleteListener onCompleteListener) {
        this.onCompleteListener = onCompleteListener;
    }

    private OnErrorListener onErrorListener = null;
    public void setOnErrorListener(OnErrorListener onErrorListener) {
        this.onErrorListener = onErrorListener;
    }

    private OnVolumeDBListener onVolumeDBListener = null;
    public void setOnVolumeDBListener(OnVolumeDBListener onVolumeDBListener) {
        this.onVolumeDBListener = onVolumeDBListener;
    }

    private OnRecordTimeListener onRecordTimeListener = null;
    public void setOnRecordTimeListener(OnRecordTimeListener onRecordTimeListener) {
        this.onRecordTimeListener = onRecordTimeListener;
    }

    private OnPcmInfoListener onPcmInfoListener = null;
    public void setOnPcmInfoListener(OnPcmInfoListener onPcmInfoListener) {
        this.onPcmInfoListener = onPcmInfoListener;
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
    }

    /**
     * 准备播放源
     */
    public void prepared(String source) {
        if (TextUtils.isEmpty(source)) {
            MyLog.i("source must not be empty");
            return;
        }

        //开启一个线程，用于底层解析音视频文件
        new Thread(new Runnable() {
            @Override
            public void run() {
                _nativePrepared(source);
            }
        }).start();
    }

    /**
     * 开始播放
     */
    public void start() {
        //开启线程，读取各个流数据packet并放入到缓存队列中
        new Thread(new Runnable() {
            @Override
            public void run() {
                setVolume(mVolumePercent_);
                setMute(mMuteEnum_);
                setPitch(mPitch_);
                setSpeed(mSpeed_);

                _nativeStart();
            }
        }).start();
    }

    /**
     * 暂停播放
     */
    public void pause() {
        _nativePause();
        if (onPauseResumeListener != null) {
            onPauseResumeListener.onPause(true);
        }
    }

    /**
     * 恢复播放
     */
    public void resume() {
        _nativeResume();
        if (onPauseResumeListener != null) {
            onPauseResumeListener.onPause(false);
        }
    }

    /**
     * 停止播放
     */
    public void stop() {
        mTimeInfoBean_ = null;
        mDuration_ = -1;

        stopAudioRecord();

        new Thread(new Runnable() {//开启一个线程，停止播放，释放底层ffmpeg的资源及释放硬解解码器的相关资源
            @Override
            public void run() {
                _nativeStop();
                _releaseVMediaCodec();
            }
        }).start();
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
        mPlayPath_ = url;
        mPlayNext_ = true;
        stop();
    }

    /**
     * 获取视频总时长
     * @return 视频总时长
     */
    public int getDuration() {
        if (mDuration_ < 0) {
            mDuration_ = _nativeDuration();
        }
        return mDuration_;
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
     * @param mute 声道类型
     */
    public void setMute(MuteEnum mute) {
        mMuteEnum_ = mute;
        _nativeMute(mute.getValue());
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
     * 开始录音
     * @param outfile 录音文件
     */
    public void startAudioRecord(File outfile) {
        if (!mIsInitMediaCodec_) {
            mAudioSamplerate_ = _nativeSamplerate();//获取音频文件的采样率
            if (mAudioSamplerate_ > 0) {
                mIsInitMediaCodec_ = true;
                initMediaCodec(mAudioSamplerate_, outfile);
                _nativeStartstopRecord(true);
                MyLog.i("开始录音....");
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
        if (mIsInitMediaCodec_) {
            _nativeStartstopRecord(false);
            _releaseVMediaCodec();
            MyLog.i("完成录音....");
        }
    }

    /**
     * 开始剪切音频播放
     * @param startTime 开始时间
     * @param endTime 结束时间
     * @param showPcm 是否显示pcm数据
     */
    public void cutAudioPlay(int startTime, int endTime, boolean showPcm) {
        if (_nativeCutAudioPlay(startTime, endTime, showPcm)) {
            start();
        } else {
            stop();
            onCallError(2001, "cutAudioPlay params is wrong!");
        }
    }

    @CalledByNative
    private void onCallLoad(boolean load) {
        if (onLoadListener != null) {
            onLoadListener.onLoad(load);
        }
    }

    @CalledByNative
    private void onCallPrepared() {
        if (onPreparedListener != null) {
            onPreparedListener.onPrepared();
        }
    }

    @CalledByNative
    private void onCallTimeInfo(int currentTime, int totalTime) {
        if (onTimeInfoListener != null) {
            if (mTimeInfoBean_ == null) {
                mTimeInfoBean_ = new TimeInfoBean();
            }
            mTimeInfoBean_.setCurrentTime(currentTime);
            mTimeInfoBean_.setTotalTime(totalTime);
            onTimeInfoListener.onTimeInfo(mTimeInfoBean_);
        }
    }

    @CalledByNative
    private void onCallComplete() {
        stop();

        if (onCompleteListener != null) {
            onCompleteListener.onComplete();
        }
    }

    @CalledByNative
    private void onCallError(int code, String msg) {
        stop();

        if (onErrorListener != null) {
            onErrorListener.onError(code, msg);
        }
    }

    @CalledByNative
    private void onCallNext() {
        MyLog.i("onCallNext playNext: " + mPlayNext_);
        if (mPlayNext_) {
            mPlayNext_ = false;
            prepared(mPlayPath_);
        }
    }

    @CalledByNative
    private void onCallVolumeDB(int db) {
        if (onVolumeDBListener != null) {
            onVolumeDBListener.onDBValue(db);
        }
    }

    @CalledByNative
    private void onCallPcmInfo(byte[] buffer, int bufferSize) {
        if (onPcmInfoListener != null) {
            onPcmInfoListener.onPcmInfo(buffer, bufferSize);
        }
    }

    @CalledByNative
    private void onCallPcmRate(int samplerate, int bit, int channels) {
        if (onPcmInfoListener != null) {
            onPcmInfoListener.onPcmRate(samplerate, bit, channels);
        }
    }

    @CalledByNative
    private void encodePcmToAAC(byte[] buffer, int size) {
        MyLog.i("encodePcmToAAC buffer size: " + size);
        if ((buffer != null) && (mEncoder_ != null)) {
            mRecordTime_ += size * 1.0 / (mAudioSamplerate_ * 2 * 2);//计算当前包的时长，并累加
            if (onRecordTimeListener != null) {
                onRecordTimeListener.onAudioRecordTime((int) mRecordTime_);//回调当前录制时长
            }

            int inputBufferIndex = mEncoder_.dequeueInputBuffer(0);//获取到编码输入buffer的可用索引
            if (inputBufferIndex >= 0) {
                ByteBuffer byteBuffer = mEncoder_.getInputBuffers()[inputBufferIndex];//根据索引获取编码输入可用的空闲buffer
                byteBuffer.clear();
                byteBuffer.put(buffer);//将pcm数据放入空用buffer中
                mEncoder_.queueInputBuffer(inputBufferIndex, 0, size, 0, 0);//编码器入队进行编码
            }

            int index = mEncoder_.dequeueOutputBuffer(mBufferInfo_, 0);//获取编码器码流输出buffer的索引
            while (index >= 0) {
                try {
                    mPerpcmSize_ = mBufferInfo_.size + 7;//AAC码流需要添加7字节的头
                    mOutByteBuffer_ = new byte[mPerpcmSize_];

                    ByteBuffer byteBuffer = mEncoder_.getOutputBuffers()[index];//获取到编码器输出的码流buffer
                    byteBuffer.position(mBufferInfo_.offset);
                    byteBuffer.limit(mBufferInfo_.offset + mBufferInfo_.size);

                    _addADTSHeader(mOutByteBuffer_, mPerpcmSize_, mAACSampleRateType_);//mediacodec编码出来的aac码流没有aac头，增加AAC码流头

                    byteBuffer.get(mOutByteBuffer_, 7, mBufferInfo_.size);//将编码码流数据放入AAC码流头后面存放
                    byteBuffer.position(mBufferInfo_.offset);

                    mFileOutputStream_.write(mOutByteBuffer_, 0, mPerpcmSize_);//将完整的一帧音频码流数据写入文件

                    mEncoder_.releaseOutputBuffer(index, false);//取出码流数据后，释放这个buffer,返回给队列中循环使用
                    index = mEncoder_.dequeueOutputBuffer(mBufferInfo_, 0);
                    mOutByteBuffer_ = null;
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    @CalledByNative
    private boolean onCallIsSupportMediaCodec(String ffcodecname) {
        MyLog.i("onCallIsSupportMediaCodec input ffcodecname: " + ffcodecname);
        boolean isSupport = WLVideoSupportUtil.isSupportCodec(ffcodecname);
        MyLog.i("onCallIsSupportMediaCodec isSupport: " + isSupport);
        return isSupport;
    }

    public static void printBytesInLines(byte[] bytes, int len) {
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
    private void onCallinitMediaCodec(String codecName, int width, int height, byte[] csd) {
        if (mSurface_ != null) {
            try {
                mWlglSurfaceView_.getWlRender().setRenderType(WLRender.RENDER_MEDIACODEC);
                mWlglSurfaceView_.getWlRender().setVideoSize(width, height);

                String mime = WLVideoSupportUtil.findVideoCodecName(codecName);
                MyLog.i("onCallinitMediaCodec mime is " + mime + " width is " + width + " height is " + height);
                mMediaFormat_ = MediaFormat.createVideoFormat(mime, width, height);
                mMediaFormat_.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, width * height);
                /*这里三个字段都是设置为ffmpeg提取的extradata数据，目前硬件解码是没问题的，理论上是需要分别提取SPS和PPS数据填充设置，
                    H265需要设置VPS，SPS，PPS三个字段.应该MediaCodec针对直接传递的extradata数据在内部进行了提取VPS,SPS,PPS,比较强大
                 */
//                MyLog.i("java onCallinitMediaCodec csd size: " + csd.length);
//                printBytesInLines(csd, csd.length);

                mMediaFormat_.setByteBuffer("csd-0", ByteBuffer.wrap(csd));
                mMediaFormat_.setByteBuffer("csd-1", ByteBuffer.wrap(csd));
                if (mime.equals("video/hevc")) {
                    mMediaFormat_.setByteBuffer("csd-2", ByteBuffer.wrap(csd));
                }
                MyLog.i(mMediaFormat_.toString());
                mMediaCodec_ = MediaCodec.createDecoderByType(mime);

                mInfo_ = new MediaCodec.BufferInfo();
                mMediaCodec_.configure(mMediaFormat_, mSurface_, null, 0);
                mMediaCodec_.start();
            } catch (Exception e) {
                e.printStackTrace();
            }

            mStartMs_ = System.currentTimeMillis();
            MyLog.i("onCallinitMediaCodec end");
        } else {
            if (onErrorListener != null) {
                onErrorListener.onError(2001, "surface is null");
            }
        }
    }

    @CalledByNative
    private void onCallRenderYUV(int width, int height, byte[] y, byte[] u, byte[] v) {
        MyLog.i("onCallRenderYUV width: " + width + " height: " + height);
        if (mWlglSurfaceView_ != null) {
            mWlglSurfaceView_.getWlRender().setRenderType(WLRender.RENDER_YUV);
            mWlglSurfaceView_.setYUVData(width, height, y, u, v);
        }
    }

   @CalledByNative
    private void onCallDecodeVPacket(int datasize, byte[] data) {
        MyLog.i("onCallDecodeVPacket in");
        /*
        *这里的码流数据，底层使用了ffmpeg进行了过滤，转换为AnnexB模式，在如果是I帧，会在I帧前补充添加带有startcode的VPS,SPS,PPS，
        * 然后再跟上实际的startcode + I帧图像数据。如果是非I帧，会直接在前面将AVCC字段转换为00 00 00 01的startcode + 实际的图像数据
        * */
//        MyLog.i("data size: " + data.length);
//        printBytesInLines(data, 1000);
        if ((mSurface_ != null) && (datasize > 0) && (data != null) && (mMediaCodec_ != null)) {
            try {
                int inputBufferIndex = mMediaCodec_.dequeueInputBuffer(10);
                if (inputBufferIndex >= 0) {
                    ByteBuffer byteBuffer = mMediaCodec_.getInputBuffers()[inputBufferIndex];
                    byteBuffer.clear();
                    byteBuffer.put(data);
                    mMediaCodec_.queueInputBuffer(inputBufferIndex, 0, datasize, 0, 0);//丢给mediaCodec解码输入队列
                }
                int outputBufferIndex = mMediaCodec_.dequeueOutputBuffer(mInfo_, 10);//循环从硬解解码器的输出队列中获取解码数据进行渲染
                while (outputBufferIndex >= 0) {
                    long decodeTime = System.currentTimeMillis() - mStartMs_;
                    mStartMs_ = System.currentTimeMillis();
                    mFrameCount_++;
                    mTotalTime_ += decodeTime;
                    mMediaCodec_.releaseOutputBuffer(outputBufferIndex, true);
                    outputBufferIndex = mMediaCodec_.dequeueOutputBuffer(mInfo_, 10);
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    private void _releaseVMediaCodec() {
        if (mMediaCodec_ != null) {
            try {
                mMediaCodec_.flush();
                mMediaCodec_.stop();
                mMediaCodec_.release();
            } catch (Exception e) {
                e.printStackTrace();
            }

            mMediaCodec_ = null;
            mMediaFormat_ = null;
            mInfo_ = null;
            MyLog.i("All the Frames: " + mFrameCount_ + " Average decode time per frame: " + (mTotalTime_ / mFrameCount_) + "ms");
        }
    }

    //音频录制的编码器创建-mediacodec
    private MediaFormat mEncoderFormat_ = null;
    private MediaCodec mEncoder_ = null;
    private FileOutputStream mFileOutputStream_ = null;
    private MediaCodec.BufferInfo mBufferInfo_ = null;
    private int mPerpcmSize_ = 0;
    private byte[] mOutByteBuffer_ = null;
    private int mAACSampleRateType_ = 4;
    private double mRecordTime_ = 0;
    private int mAudioSamplerate_ = 0;

    private void initMediaCodec(int samplerate, File outfile) {
        try {
            mAACSampleRateType_ = _getADTSSampleRate(samplerate);//根据音频采样率得到填充ADTS的采样率对应的值

            mEncoderFormat_ = MediaFormat.createAudioFormat(MediaFormat.MIMETYPE_AUDIO_AAC, samplerate, 2);
            mEncoderFormat_.setInteger(MediaFormat.KEY_BIT_RATE, 96000);//码率
            mEncoderFormat_.setInteger(MediaFormat.KEY_AAC_PROFILE, MediaCodecInfo.CodecProfileLevel.AACObjectLC);//AAC profile
            mEncoderFormat_.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, 4096);//输入编码的最大pcm数据大小
            mEncoder_ = MediaCodec.createEncoderByType(MediaFormat.MIMETYPE_AUDIO_AAC);//创建音频编码器
            mBufferInfo_ = new MediaCodec.BufferInfo();
            if (mEncoder_ == null) {
                MyLog.e("create encoder wrong");
                return;
            }

            mRecordTime_ = 0;

            mEncoder_.configure(mEncoderFormat_, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);//配置编码器
            mFileOutputStream_ = new FileOutputStream(outfile);//创建写文件输出流
            mEncoder_.start();//启动音频编码器
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    //为每个AAC码流包增加ADTS头
    private void _addADTSHeader(byte[] packet, int packetLen, int samplerate) {
        int profile = 2; // AAC LC
        int freqIdx = samplerate; // samplerate
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
    private int _getADTSSampleRate(int samplerate) {
        int rate = 4;
        switch (samplerate) {
            case 96000:
                rate = 0;
                break;
            case 88200:
                rate = 1;
                break;
            case 64000:
                rate = 2;
                break;
            case 48000:
                rate = 3;
                break;
            case 44100:
                rate = 4;
                break;
            case 32000:
                rate = 5;
                break;
            case 24000:
                rate = 6;
                break;
            case 22050:
                rate = 7;
                break;
            case 16000:
                rate = 8;
                break;
            case 12000:
                rate = 9;
                break;
            case 11025:
                rate = 10;
                break;
            case 8000:
                rate = 11;
                break;
            case 7350:
                rate = 12;
                break;
        }
        return rate;
    }

    //释放音频编码器
    private void _releaseAMediaCodec() {
        if (mEncoder_ == null) {
            return;
        }

        try {
            mRecordTime_ = 0;

            mFileOutputStream_.close();
            mFileOutputStream_ = null;

            mEncoder_.stop();
            mEncoder_.release();
            mEncoder_ = null;
            mEncoderFormat_ = null;
            mBufferInfo_ = null;

            mIsInitMediaCodec_ = false;
            MyLog.i("录制完成....");
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    //native方法
    private native void _nativePrepared(String source);

    private native void _nativeStart();

    private native void _nativePause();

    private native void _nativeResume();

    private native void _nativeStop();

    private native void _nativeSeek(int secds);

    private native int _nativeDuration();

    private native void _nativeVolume(int percent);

    private native void _nativeMute(int mute);

    private native void _nativePitch(float pitch);

    private native void _nativeSpeed(float speed);

    private native int _nativeSamplerate();

    private native void _nativeStartstopRecord(boolean start);

    private native boolean _nativeCutAudioPlay(int start_time, int end_time, boolean showPcm);
}
