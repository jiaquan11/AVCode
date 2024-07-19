package com.jiaquan.myplayer.demo;

import android.content.Context;
import android.media.MediaCodec;
import android.media.MediaFormat;
import android.util.Log;
import android.view.Surface;

import com.jiaquan.myplayer.R;

import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;

/**
 * 开启一个线程，码流文件的码流数据提取及硬件解码，
 * 只是测试流程，不进行画面渲染操作，并统计了平均解码耗时
 * 这里是使用直接读取视频码流数据，裸流文件
 */
public class VideoDataPlayTest extends Thread {
    private final String TAG = VideoDataPlayTest.class.getSimpleName();

    private Context mContext_ = null;
    private MediaCodec mVideoDecoder_ = null;
    private String mMine_ = "video/hevc";
    private long mTotalTime_ = 0;
    private int mFrameCount_ = 0;
    private Surface mSurface_ = null;
    private MediaFormat mVideoFormat_ = null;
    private int mWidth_ = 1080;
    private int mHeight_ = 1920;
    private byte[] mBytes_ = null;
    private int mAvgTime_ = 0;

    public VideoDataPlayTest(Context applicationContext) {
        mContext_ = applicationContext;
    }

    public void setSurface(Surface s) {
        mSurface_ = s;
    }

    public int getWidth() {
        return mWidth_;
    }

    public int getHeight() {
        return mHeight_;
    }

    @Override
    public void run() {
//        try {
//            bytes = _getBytes("/sdcard/testziliao/hanleiVideo.265");
//        } catch (IOException e) {
//            e.printStackTrace();
//        }
//        Log.i(TAG, "sd resource bytes size " + bytes.length);
        InputStream is = mContext_.getResources().openRawResource(R.raw.test30frames_1080p_ld2);
        try {
            mBytes_ = _getBytes(is);
        } catch (IOException e) {
            e.printStackTrace();
        }

        try {
            mVideoDecoder_ = MediaCodec.createDecoderByType(mMine_);
        } catch (IOException e) {
            e.printStackTrace();
            mVideoDecoder_ = null;
        }
        if (mVideoDecoder_ == null) {
            return;
        }

        mVideoFormat_ = MediaFormat.createVideoFormat(mMine_, mWidth_, mHeight_);
        mVideoFormat_.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, mWidth_ * mHeight_);
        mVideoFormat_.setInteger(MediaFormat.KEY_FRAME_RATE, 30);//设置无效
        mVideoDecoder_.configure(mVideoFormat_, mSurface_, null, 0);
        mVideoDecoder_.start();

        //开始位置
        int startIndex = 0;
        int totalSize = mBytes_.length;
        MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();
        long startMs = System.currentTimeMillis();
        boolean isEOS = false;
        while (!Thread.interrupted()) {
            if ((totalSize == 0) || (startIndex >= totalSize)) {
                break;
            }

            /**
             寻找索引
             这里只会寻找00 00 00 01然后进行读取，也就是送给解码器的可能是
             00 00 00 01 + VPS，或者是00 00 00 01 + SPS，或者是00 00 00 01 + PPS
             或者是00 00 00 01 + startcode + 帧实际的码流图像,即使单独送入VPS SPS PPS，
             分别单独的送入解码器，这样也是可以正常解码并渲染。
             也就是并不需要在配置解码器之前进行单独设置VPS SPS PPS
             mediaFormat.setByteBuffer("csd-0", ByteBuffer.wrap(csd));
             mediaFormat.setByteBuffer("csd-1", ByteBuffer.wrap(csd));
             if (mime.equals("video/hevc")) {
                mediaFormat.setByteBuffer("csd-2", ByteBuffer.wrap(csd));
             }
            */
            int nextFrameStart = _findByFrame(mBytes_, startIndex + 1, totalSize);
            if (!isEOS) {
                int inIndex = mVideoDecoder_.dequeueInputBuffer(10000);
                if (inIndex >= 0) {
                    ByteBuffer buffer = null;
                    if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.LOLLIPOP) {
                        buffer = mVideoDecoder_.getInputBuffer(inIndex);
                        buffer.clear();
                    }

                    Log.i(TAG, "nextFrameStart " + nextFrameStart);
                    if (nextFrameStart == -1) {
                        mVideoDecoder_.queueInputBuffer(inIndex, 0, 0, 0, MediaCodec.BUFFER_FLAG_END_OF_STREAM);
                        Log.i(TAG, "read to the end of the stream");
                        isEOS = true;
                    } else {
                        Log.i(TAG, "video size: " + (nextFrameStart - startIndex));
                        buffer.put(mBytes_, startIndex, nextFrameStart - startIndex);
                        mVideoDecoder_.queueInputBuffer(inIndex, 0, nextFrameStart - startIndex, 0, 0);
                    }
                    //为下一帧做准备，下一帧首就是前一帧的尾
                    startIndex = nextFrameStart;
                }
            }

            int outIndex = mVideoDecoder_.dequeueOutputBuffer(info, 10000);
            switch (outIndex) {
                case MediaCodec.INFO_OUTPUT_FORMAT_CHANGED:
                    break;
                case MediaCodec.INFO_TRY_AGAIN_LATER:
                    break;
                case MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED:
                    break;
                default:
                    long decodeTime = System.currentTimeMillis() - startMs;
                    startMs = System.currentTimeMillis();
                    mFrameCount_++;
                    mTotalTime_ += decodeTime;
                    mVideoDecoder_.releaseOutputBuffer(outIndex, true);
                    break;
            }
            if ((info.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
                Log.i(TAG, "the MediaCodec is all finished");
                break;
            }
        }
        mVideoDecoder_.stop();
        mVideoDecoder_.release();
        mAvgTime_ = (int)(mTotalTime_ / mFrameCount_);
        Log.i(TAG, "All the Frames: " + mFrameCount_ + ", Average decode time per frame: " + mAvgTime_ + "ms");
    }

    /**
     * 根据分隔符查找帧的位置
     */
    private int _findByFrame(byte[] bytes, int start, int totalSize) {
        for (int i = start; i < totalSize - 4; i++) {
            //对output.h265文件分析 可通过分隔符 0x00000001 读取真正的数据
            if ((bytes[i] == 0x00) && (bytes[i + 1] == 0x00) && (bytes[i + 2] == 0x00) && (bytes[i + 3] == 0x01)) {
                return i;
            }
        }
        return -1;
    }

    /**
     * 读取文件内容到输出流中(以文件路径的方式)
     */
    private byte[] _getBytes(String videoPath) throws IOException {
        InputStream is = new DataInputStream(new FileInputStream(new File(videoPath)));
        int bytesRead;
        int bufferSize = 8192;//8KB缓冲区大小
        byte[] buffer = new byte[bufferSize];
        ByteArrayOutputStream bos = new ByteArrayOutputStream();//输出流
        while ((bytesRead = is.read(buffer, 0, bufferSize)) != -1) {
            bos.write(buffer, 0, bytesRead);
        }
        return bos.toByteArray();
    }

    /**
     * 读取文件内容到输出流中(以输入流的方式)
     */
    private byte[] _getBytes(InputStream is) throws IOException {
        int bytesRead;
        int bufferSize = 8192;//8KB缓冲区大小
        ByteArrayOutputStream bos = new ByteArrayOutputStream();//输出流
        byte[] buffer = new byte[bufferSize];
        while ((bytesRead = is.read(buffer, 0, bufferSize)) != -1) {
            bos.write(buffer, 0, bytesRead);
        }
        return bos.toByteArray();
    }
}
