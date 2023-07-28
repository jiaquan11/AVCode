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
import com.jiaquan.myplayer.util.TimeInfoBean;
import com.jiaquan.myplayer.util.WLVideoSupportUtil;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;

public class WLPlayer {
    private final String TAG = WLPlayer.class.getSimpleName();

    //加载需要的动态库
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

    private static String sourcePath = null;
    private static boolean playNext = false;
    private static int duration = -1;
    private static int volumePercent = 100;
    private static MuteEnum muteEnum = MuteEnum.MUTE_CENTER;
    private static float speed = 1.0f;
    private static float pitch = 1.0f;
    private static boolean isInitMediaCodec = false;

    private MediaFormat mediaFormat = null;
    private MediaCodec mediaCodec = null;
    private Surface surface = null;
    private MediaCodec.BufferInfo info = null;
    public long mTotalTime = 0;//记录硬解耗时
    public int mFrameCount = 0;//记录硬解播放的总帧数
    private long mStartMs = 0;//记录每次硬解解码前的系统时间

    private static TimeInfoBean timeInfoBean = null;
    private WLGLSurfaceView wlglSurfaceView = null;

    //设置播放渲染控件
    public void setWlglSurfaceView(WLGLSurfaceView wlglSurfaceView) {
        this.wlglSurfaceView = wlglSurfaceView;
        wlglSurfaceView.getWlRender().setOnSurfaceCreateListener(new WLRender.OnSurfaceCreateListener() {
            @Override
            public void onSurfaceCreate(Surface s) {
                if (surface == null) {
                    surface = s;//监听获取到的surface,用于配置给硬件解码器
                    MyLog.i("onSurfaceCreate get surface");
                }
            }
        });
    }

    //监听设置
    private OnPreparedListener onPreparedListener = null;
    public void setOnPreparedListener(OnPreparedListener onPreparedListener) {
        this.onPreparedListener = onPreparedListener;
    }

    private OnLoadListener onLoadListener = null;
    public void setOnLoadListener(OnLoadListener onLoadListener) {
        this.onLoadListener = onLoadListener;
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

    private OnPcmInfoListener onPcmInfoListener = null;

    public void setOnPcmInfoListener(OnPcmInfoListener onPcmInfoListener) {
        this.onPcmInfoListener = onPcmInfoListener;
    }

    private OnRecordTimeListener onRecordTimeListener = null;
    public void setOnRecordTimeListener(OnRecordTimeListener onRecordTimeListener) {
        this.onRecordTimeListener = onRecordTimeListener;
    }

    @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
    private void getSupportedCodec() {
        MediaCodecList list = new MediaCodecList(MediaCodecList.REGULAR_CODECS);
        MediaCodecInfo[] codecs = list.getCodecInfos();
        MyLog.i("Decoders:");
        for (MediaCodecInfo codec : codecs) {
            if (!codec.isEncoder()) {
                //基本就是一个name对应一个type，但是多个name可能对应的是同一个type
                String[] types = codec.getSupportedTypes();
                for (String type : types) {
                    MyLog.i("Name: " + codec.getName() + ", Type: " + type + ", isHardwareAccelerated: " + codec.isHardwareAccelerated());
                }
            }
        }
        MyLog.i("Encoders:");
        for (MediaCodecInfo codec : codecs) {
            if (codec.isEncoder()) {
                String[] types = codec.getSupportedTypes();
                for (String type : types) {
                    MyLog.i("Name: " + codec.getName() + ", Type: " + type + ", isHardwareAccelerated: " + codec.isHardwareAccelerated());
                }
            }
        }
    }

    //构造函数
    public WLPlayer() {
        //仅用于测试：打印获取该机器支持的编解码器类型
        MyLog.i("print getSupportCodec");
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            getSupportedCodec();
        }
    }

    public void setSource(String source) {
        sourcePath = source;
    }

    //准备资源，解封装媒体文件头
    public void prepared() {
        if (TextUtils.isEmpty(sourcePath)) {//检测播放路径是否为空
            MyLog.i("source must not be empty");
            return;
        }
//        onCallLoad(true);

        new Thread(new Runnable() {//开启一个线程，用于native层解封装文件头
            @Override
            public void run() {
                _prepared(sourcePath);
            }
        }).start();
    }

    //准备好资源后，开始播放
    public void start() {
        new Thread(new Runnable() {//开启一个线程，用于底层读取各个流数据packet并放入到缓存队列中
            @Override
            public void run() {
                //开始播放前，先把保存的初始化值进行设置到native
                setVolume(volumePercent);
                setMute(muteEnum);
                setPitch(pitch);
                setSpeed(speed);

                _start();
            }
        }).start();
    }

    //暂停播放
    public void pause() {
        _pause();

        //暂停后直接回调
        if (onPauseResumeListener != null) {
            onPauseResumeListener.onPause(true);
        }
    }

    //恢复播放
    public void resume() {
        _resume();

        //恢复播放后直接回调
        if (onPauseResumeListener != null) {
            onPauseResumeListener.onPause(false);
        }
    }

    //停止播放
    public void stop() {
        timeInfoBean = null;
        duration = -1;

        stopRecord();

        new Thread(new Runnable() {//开启一个线程，停止播放，释放底层ffmpeg的资源及释放硬解解码器的相关资源
            @Override
            public void run() {
                _stop();
                releaseVMediaCodec();
            }
        }).start();
    }

    //播放seek操作
    public void seek(int secds) {
        _seek(secds);
    }

    //切换下一个播放资源
    public void playNext(String url) {
        sourcePath = url;
        playNext = true;
        stop();
    }

    //获取总时长
    public int getDuration() {
        if (duration < 0) {
            duration = _duration();
        }
        return duration;
    }

    //设置音量值
    public void setVolume(int percent) {
        if ((percent >= 0) && (percent <= 100)) {
            volumePercent = percent;
            _volume(percent);
        }
    }

    //获取当前音量值
    public int getVolumePercent() {
        return volumePercent;
    }

    //设置控制的左右声道
    public void setMute(MuteEnum mute) {
        muteEnum = mute;
        _mute(mute.getValue());
    }

    //设置音调
    public void setPitch(float p) {
        pitch = p;
        _pitch(pitch);
    }

    //设置音频播放速度
    public void setSpeed(float s) {
        speed = s;
        _speed(speed);
    }

    //开始音频录制，创建音频编码器
    public void startRecord(File outfile) {
        if (!isInitMediaCodec) {
            audioSamplerate = _samplerate();//获取音频文件的采样率
            if (audioSamplerate > 0) {
                isInitMediaCodec = true;
                initMediaCodec(audioSamplerate, outfile);
                _startstopRecord(true);
                MyLog.i("开始录制....");
            }
        }
    }

    //暂停录制
    public void pauseRecord() {
        _startstopRecord(false);
        MyLog.i("暂停录制....");
    }

    //恢复录制
    public void resumeRecord() {
        _startstopRecord(true);
        MyLog.i("恢复录制....");
    }

    //停止录制
    public void stopRecord() {
        if (isInitMediaCodec) {
            _startstopRecord(false);
            releaseAMediaCodec();
            MyLog.i("完成录制....");
        }
    }

    //裁剪音频
    public void cutAudioPlay(int start_time, int end_time, boolean showPcm) {
        if (_cutAudioPlay(start_time, end_time, showPcm)) {//先seek
            start();//然后提取数据上报
        } else {
            stop();
            onCallError(2001, "cutAudioPlay params is wrong!");
        }
    }

    //native回调方法
    //native回调方法：回调播放器资源已准备
    private void onCallPrepared() {
        if (onPreparedListener != null) {
            onPreparedListener.onPrepared();
        }
    }

    //native回调方法：回调播放器资源已准备
    private void onCallLoad(boolean load) {
        if (onLoadListener != null) {
            onLoadListener.onLoad(load);
        }
    }

    //native回调方法：返回当前播放时间戳，用于进度条显示
    private void onCallTimeInfo(int currentTime, int totalTime) {
        if (onTimeInfoListener != null) {
            if (timeInfoBean == null) {
                timeInfoBean = new TimeInfoBean();
            }
            timeInfoBean.setCurrentTime(currentTime);
            timeInfoBean.setTotalTime(totalTime);
            onTimeInfoListener.onTimeInfo(timeInfoBean);
        }
    }

    //native回调方法:回调播放完成
    private void onCallComplete() {
        stop();

        if (onCompleteListener != null) {
            onCompleteListener.onComplete();
        }
    }

    //native回调方法:回调错误
    private void onCallError(int code, String msg) {
        stop();

        if (onErrorListener != null) {
            onErrorListener.onError(code, msg);
        }
    }

    //native回调方法:
    private void onCallNext() {
        MyLog.i("onCallNext playNext: " + playNext);
        if (playNext) {
            playNext = false;
            prepared();
        }
    }

    //native回调方法:回调音量分贝值
    private void onCallVolumeDB(int db) {
        if (onVolumeDBListener != null) {
            onVolumeDBListener.onDBValue(db);
        }
    }

    //native回调方法:回调音频解码的pcm数据
    private void onCallPcmInfo(byte[] buffer, int bufferSize) {
        if (onPcmInfoListener != null) {
            onPcmInfoListener.onPcmInfo(buffer, bufferSize);
        }
    }

    //native回调方法:回调音频参数信息
    private void onCallPcmRate(int samplerate, int bit, int channels) {
        if (onPcmInfoListener != null) {
            onPcmInfoListener.onPcmRate(samplerate, bit, channels);
        }
    }

    //native回调方法:回调音频解码pcm数据，用于音频编码
    private void encodePcmToAAC(byte[] buffer, int size) {
        MyLog.i("encodePcmToAAC buffer size: " + size);
        if ((buffer != null) && (encoder != null)) {
            recordTime += size * 1.0 / (audioSamplerate * 2 * 2);//计算当前包的时长，并累加
            if (onRecordTimeListener != null) {
                onRecordTimeListener.onRecordTime((int) recordTime);//回调当前录制时长
            }

            int inputBufferIndex = encoder.dequeueInputBuffer(0);//获取到编码输入buffer的可用索引
            if (inputBufferIndex >= 0) {
                ByteBuffer byteBuffer = encoder.getInputBuffers()[inputBufferIndex];//根据索引获取编码输入可用的空闲buffer
                byteBuffer.clear();
                byteBuffer.put(buffer);//将pcm数据放入空用buffer中
                encoder.queueInputBuffer(inputBufferIndex, 0, size, 0, 0);//编码器入队进行编码
            }

            int index = encoder.dequeueOutputBuffer(bufferInfo, 0);//获取编码器码流输出buffer的索引
            while (index >= 0) {
                try {
                    perpcmSize = bufferInfo.size + 7;//AAC码流需要添加7字节的头
                    outByteBuffer = new byte[perpcmSize];

                    ByteBuffer byteBuffer = encoder.getOutputBuffers()[index];//获取到编码器输出的码流buffer
                    byteBuffer.position(bufferInfo.offset);
                    byteBuffer.limit(bufferInfo.offset + bufferInfo.size);

                    addADTSHeader(outByteBuffer, perpcmSize, aacSampleRateType);//mediacodec编码出来的aac码流没有aac头，增加AAC码流头

                    byteBuffer.get(outByteBuffer, 7, bufferInfo.size);//将编码码流数据放入AAC码流头后面存放
                    byteBuffer.position(bufferInfo.offset);

                    fileOutputStream.write(outByteBuffer, 0, perpcmSize);//将完整的一帧音频码流数据写入文件

                    encoder.releaseOutputBuffer(index, false);//取出码流数据后，释放这个buffer,返回给队列中循环使用
                    index = encoder.dequeueOutputBuffer(bufferInfo, 0);
                    outByteBuffer = null;
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    //native回调方法：判断是否支持硬解指定的解码器
    private boolean onCallIsSupportMediaCodec(String ffcodecname) {
        MyLog.i("onCallIsSupportMediaCodec input ffcodecname: " + ffcodecname);
        boolean isSupport = WLVideoSupportUtil.isSupportCodec(ffcodecname);
        MyLog.i("onCallIsSupportMediaCodec isSupport: " + isSupport);
        return isSupport;
    }

    //video
    //native回调方法：初始化视频硬件解码器
    private void onCallinitMediaCodec(String codecName, int width, int height, byte[] csd) {
        if (surface != null) {
            try {
                wlglSurfaceView.getWlRender().setRenderType(WLRender.RENDER_MEDIACODEC);
                wlglSurfaceView.getWlRender().setVideoSize(width, height);

                String mime = WLVideoSupportUtil.findVideoCodecName(codecName);
                MyLog.i("onCallinitMediaCodec mime is " + mime + " width is " + width + " height is " + height);
                mediaFormat = MediaFormat.createVideoFormat(mime, width, height);
                mediaFormat.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, width * height);
                /*这里三个字段都是设置为ffmpeg提取的extradata数据，目前硬件解码是没问题的，理论上是需要分别提取SPS和PPS数据填充设置，
                    H265需要设置VPS，SPS，PPS三个字段
                 */
                mediaFormat.setByteBuffer("csd-0", ByteBuffer.wrap(csd));
                mediaFormat.setByteBuffer("csd-1", ByteBuffer.wrap(csd));
                if (mime.equals("video/hevc")) {
                    mediaFormat.setByteBuffer("csd-2", ByteBuffer.wrap(csd));
                }
                MyLog.i(mediaFormat.toString());
                mediaCodec = MediaCodec.createDecoderByType(mime);

                info = new MediaCodec.BufferInfo();
                mediaCodec.configure(mediaFormat, surface, null, 0);//硬件解码器配置
                mediaCodec.start();//开始解码
            } catch (Exception e) {
                e.printStackTrace();
            }

            mStartMs = System.currentTimeMillis();
            MyLog.i("onCallinitMediaCodec end");
        } else {
            if (onErrorListener != null) {
                onErrorListener.onError(2001, "surface is null");
            }
        }
    }

    //native回调方法：传递YUV数据用于上层渲染
    private void onCallRenderYUV(int width, int height, byte[] y, byte[] u, byte[] v) {
        MyLog.i("onCallRenderYUV width: " + width + " height: " + height);
        if (wlglSurfaceView != null) {
            wlglSurfaceView.getWlRender().setRenderType(WLRender.RENDER_YUV);
            wlglSurfaceView.setYUVData(width, height, y, u, v);
        }
    }

    //底层回调方法：硬解解码底层回调的码流数据包，并直接渲染到绑定的surface上面
    private void onCallDecodeVPacket(int datasize, byte[] data) {
        MyLog.i("onCallDecodeVPacket in");
        if ((surface != null) && (datasize > 0) && (data != null) && (mediaCodec != null)) {
            try {
                int inputBufferIndex = mediaCodec.dequeueInputBuffer(10);
                if (inputBufferIndex >= 0) {
                    ByteBuffer byteBuffer = mediaCodec.getInputBuffers()[inputBufferIndex];
                    byteBuffer.clear();
                    byteBuffer.put(data);
                    mediaCodec.queueInputBuffer(inputBufferIndex, 0, datasize, 0, 0);//丢给mediaCodec解码输入队列
                }
                int outputBufferIndex = mediaCodec.dequeueOutputBuffer(info, 10);//循环从硬解解码器的输出队列中获取解码数据进行渲染
                while (outputBufferIndex >= 0) {
                    long decodeTime = System.currentTimeMillis() - mStartMs;
                    mStartMs = System.currentTimeMillis();
                    mFrameCount++;
                    mTotalTime += decodeTime;
                    mediaCodec.releaseOutputBuffer(outputBufferIndex, true);
                    outputBufferIndex = mediaCodec.dequeueOutputBuffer(info, 10);
                    MyLog.i("mediaCodec releaseOutputBuffer");
                }
                MyLog.i("onCallDecodeVPacket out");
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    private void releaseVMediaCodec() {
        if (mediaCodec != null) {
            try {
                mediaCodec.flush();
                mediaCodec.stop();
                mediaCodec.release();
            } catch (Exception e) {
                e.printStackTrace();
            }

            mediaCodec = null;
            mediaFormat = null;
            info = null;
            MyLog.i("All the Frames: " + mFrameCount + " Average decode time per frame: " + (mTotalTime / mFrameCount) + "ms");
        }
    }

    //音频录制的编码器创建-mediacodec
    private MediaFormat encoderFormat = null;
    private MediaCodec encoder = null;
    private FileOutputStream fileOutputStream = null;
    private MediaCodec.BufferInfo bufferInfo = null;
    private int perpcmSize = 0;
    private byte[] outByteBuffer = null;
    private int aacSampleRateType = 4;
    private double recordTime = 0;
    private int audioSamplerate = 0;

    private void initMediaCodec(int samplerate, File outfile) {
        try {
            aacSampleRateType = getADTSSampleRate(samplerate);//根据音频采样率得到填充ADTS的采样率对应的值

            encoderFormat = MediaFormat.createAudioFormat(MediaFormat.MIMETYPE_AUDIO_AAC, samplerate, 2);
            encoderFormat.setInteger(MediaFormat.KEY_BIT_RATE, 96000);//码率
            encoderFormat.setInteger(MediaFormat.KEY_AAC_PROFILE, MediaCodecInfo.CodecProfileLevel.AACObjectLC);//AAC profile
            encoderFormat.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, 4096);//输入编码的最大pcm数据大小
            encoder = MediaCodec.createEncoderByType(MediaFormat.MIMETYPE_AUDIO_AAC);//创建音频编码器
            bufferInfo = new MediaCodec.BufferInfo();
            if (encoder == null) {
                MyLog.e("create encoder wrong");
                return;
            }

            recordTime = 0;

            encoder.configure(encoderFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);//配置编码器
            fileOutputStream = new FileOutputStream(outfile);//创建写文件输出流
            encoder.start();//启动音频编码器
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    //为每个AAC码流包增加ADTS头
    private void addADTSHeader(byte[] packet, int packetLen, int samplerate) {
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
    private int getADTSSampleRate(int samplerate) {
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
    private void releaseAMediaCodec() {
        if (encoder == null) {
            return;
        }

        try {
            recordTime = 0;

            fileOutputStream.close();
            fileOutputStream = null;

            encoder.stop();
            encoder.release();
            encoder = null;
            encoderFormat = null;
            bufferInfo = null;

            isInitMediaCodec = false;
            MyLog.i("录制完成....");
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            if (fileOutputStream != null) {
                try {
                    fileOutputStream.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
                fileOutputStream = null;
            }
        }
    }

    //native方法
    private native void _prepared(String source);

    private native void _start();

    private native void _pause();

    private native void _resume();

    private native void _stop();

    private native void _seek(int secds);

    private native int _duration();

    private native void _volume(int percent);

    private native void _mute(int mute);

    private native void _pitch(float pitch);

    private native void _speed(float speed);

    private native int _samplerate();

    private native void _startstopRecord(boolean start);

    private native boolean _cutAudioPlay(int start_time, int end_time, boolean showPcm);
}
