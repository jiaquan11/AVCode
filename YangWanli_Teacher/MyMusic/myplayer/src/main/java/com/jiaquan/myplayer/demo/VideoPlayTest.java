package com.jiaquan.myplayer.demo;

import android.media.MediaCodec;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.util.Log;
import android.view.Surface;

import java.io.IOException;
import java.nio.ByteBuffer;

/**
 * 使用MediaExtractor提取视频文件中的码流数据，
 * 使用MediaCodec解码视频数据，使用Surface显示解码后的视频数据
 */
public class VideoPlayTest extends Thread {
    private final String TAG = VideoPlayTest.class.getSimpleName();

    private MediaExtractor mExtractor_ = null;
    private MediaCodec mVideoDecoder_ = null;
    private long mTotalTime_ = 0;
    private int mFrameCount_ = 0;
    private Surface mSurface_ = null;
    private MediaFormat mVideoFormat_ = null;
    private String mMine_ = null;

    public VideoPlayTest() {
        mExtractor_ = new MediaExtractor();
        try {
            mExtractor_.setDataSource("/sdcard/testziliao/hanleiVideo.mp4");
        } catch (IOException e) {
            e.printStackTrace();
        }

        for (int i = 0; i < mExtractor_.getTrackCount(); i++) {
            MediaFormat format = mExtractor_.getTrackFormat(i);
            String mime = format.getString(MediaFormat.KEY_MIME);
            Log.i(TAG, "mime is " + mime);
            if (mime.startsWith("video/")) {
                mExtractor_.selectTrack(i);
                mVideoFormat_ = format;
                mMine_ = mime;
                break;
            }
        }
    }

    public void setSurface(Surface s) {
        mSurface_ = s;
    }

    public int getWidth() {
        return mVideoFormat_.getInteger(MediaFormat.KEY_WIDTH);
    }

    public int getHeight() {
        return mVideoFormat_.getInteger(MediaFormat.KEY_HEIGHT);
    }

    @Override
    public void run() {
        try {
            mVideoDecoder_ = MediaCodec.createDecoderByType(mMine_);
        } catch (IOException e) {
            e.printStackTrace();
        }

        if (mVideoDecoder_ == null) {
            return;
        }

        mVideoDecoder_.configure(mVideoFormat_, mSurface_, null, 0);
        mVideoDecoder_.start();

        MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();
        boolean isEOS = false;
        long startMs = System.currentTimeMillis();
        while (!Thread.interrupted()) {
            if (!isEOS) {
                int inIndex = mVideoDecoder_.dequeueInputBuffer(10000);
                if (inIndex >= 0) {
                    ByteBuffer buffer = null;
                    if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.LOLLIPOP) {
                        buffer = mVideoDecoder_.getInputBuffer(inIndex);
                    }
                    int sampleSize = mExtractor_.readSampleData(buffer, 0);
                    Log.i(TAG, "sampleSize is: " + sampleSize);
                    if (sampleSize <= 0) {
                        mVideoDecoder_.queueInputBuffer(inIndex, 0, 0, 0, MediaCodec.BUFFER_FLAG_END_OF_STREAM);
                        isEOS = true;
                        Log.i(TAG, "the end of the stream");
                    } else {
                        mVideoDecoder_.queueInputBuffer(inIndex, 0, sampleSize, mExtractor_.getSampleTime(), 0);
                        mExtractor_.advance();
                    }
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
        mExtractor_.release();
        Log.i(TAG, "All the Frames: " + mFrameCount_ + " Average decode time per frame: " + (mTotalTime_ / mFrameCount_) + "ms");
    }
}

