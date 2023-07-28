package com.jiaquan.myplayer.demo;

import android.media.MediaCodec;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.util.Log;
import android.view.Surface;

import java.io.IOException;
import java.nio.ByteBuffer;

/*
 * 开启一个线程，执行视频文件的码流提取及硬件解码，
 * 只是测试流程，不进行画面渲染操作，并统计了平均解码耗时
 * 这里是使用MediaExtractor提取视频码流数据，只能针对视频文件
 * */
public class VideoPlayTest extends Thread {
    private final String TAG = VideoPlayTest.class.getSimpleName();

    private MediaExtractor extractor = null;
    private MediaCodec decoder = null;
    private long totalTime = 0;
    private int frameCount = 0;
    private Surface surface = null;
    private MediaFormat mVideoFormat = null;
    private String mMine = null;

    public VideoPlayTest() {
        extractor = new MediaExtractor();
        try {
            extractor.setDataSource("/sdcard/testziliao/hanleiVideo.mp4");
        } catch (IOException e) {
            e.printStackTrace();
        }

        for (int i = 0; i < extractor.getTrackCount(); i++) {
            MediaFormat format = extractor.getTrackFormat(i);
            String mime = format.getString(MediaFormat.KEY_MIME);
            Log.i(TAG, "mime is " + mime);
            if (mime.startsWith("video/")) {
                extractor.selectTrack(i);
                mVideoFormat = format;
                mMine = mime;
                break;
            }
        }
    }

    public void setSurface(Surface s) {
        surface = s;
    }

    public int getWidth() {
        return mVideoFormat.getInteger(MediaFormat.KEY_WIDTH);
    }
    public int getHeight() {
        return mVideoFormat.getInteger(MediaFormat.KEY_HEIGHT);
    }

    @Override
    public void run() {
        try {
            decoder = MediaCodec.createDecoderByType(mMine);
        } catch (IOException e) {
            e.printStackTrace();
        }

        if (decoder == null) {
            return;
        }

        decoder.configure(mVideoFormat, surface, null, 0);
        decoder.start();

        MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();
        boolean isEOS = false;
        long startMs = System.currentTimeMillis();
        while (!Thread.interrupted()) {
            if (!isEOS) {
                int inIndex = decoder.dequeueInputBuffer(10000);
                if (inIndex >= 0) {
                    ByteBuffer buffer = null;
                    if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.LOLLIPOP) {
                        buffer = decoder.getInputBuffer(inIndex);
                    }
                    int sampleSize = extractor.readSampleData(buffer, 0);
                    Log.i(TAG, "sampleSize is: " + sampleSize);
                    if (sampleSize <= 0) {
                        decoder.queueInputBuffer(inIndex, 0, 0, 0, MediaCodec.BUFFER_FLAG_END_OF_STREAM);
                        isEOS = true;
                    } else {
                        decoder.queueInputBuffer(inIndex, 0, sampleSize, extractor.getSampleTime(), 0);
                        extractor.advance();
                    }
                }
            }

            int outIndex = decoder.dequeueOutputBuffer(info, 10000);
//            Log.i(TAG, "outIndex: " + outIndex);
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
                    frameCount++;
                    totalTime += decodeTime;
                    decoder.releaseOutputBuffer(outIndex, true);
                    break;
            }

            if ((info.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
                break;
            }
        }

        decoder.stop();
        decoder.release();
        extractor.release();
        Log.i(TAG, "All the Frames: " + frameCount + " Average decode time per frame: " + (totalTime / frameCount) + "ms");
    }
}

