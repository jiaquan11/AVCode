package com.jiaquan.myplayer.demo;

import android.media.MediaCodec;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.util.Log;

import java.io.IOException;
import java.nio.ByteBuffer;

/*
* 开启一个线程，执行H265视频文件的码流提取及硬件解码，
* 只是测试流程，不进行画面渲染操作，并统计了平均解码耗时
* */
public class H265VideoPlayTest extends Thread {
    private final String TAG = H265VideoPlayTest.class.getSimpleName();

    private MediaExtractor extractor = null;
    private MediaCodec decoder = null;
    private long totalTime = 0;
    private int frameCount = 0;

    @Override
    public void run() {
        extractor = new MediaExtractor();
        try {
            try {
                extractor.setDataSource("/sdcard/testziliao/hanleiVideo.mp4");
            } catch (IOException e) {
                e.printStackTrace();
            }
            for (int i = 0; i < extractor.getTrackCount(); i++) {
                MediaFormat format = extractor.getTrackFormat(i);
                String mime = format.getString(MediaFormat.KEY_MIME);
                if (mime.startsWith("video/")) {
                    extractor.selectTrack(i);
                    decoder = MediaCodec.createDecoderByType(mime);
                    decoder.configure(format, null, null, 0);
                    break;
                }
            }
        } catch (IOException e) {
            e.printStackTrace();
        }

        if (decoder == null) {
            return;
        }

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
                    if (sampleSize < 0) {
                        decoder.queueInputBuffer(inIndex, 0, 0, 0, MediaCodec.BUFFER_FLAG_END_OF_STREAM);
                        isEOS = true;
                    } else {
                        decoder.queueInputBuffer(inIndex, 0, sampleSize, extractor.getSampleTime(), 0);
                        extractor.advance();
                    }
                }
            }

            int outIndex = decoder.dequeueOutputBuffer(info, 10000);
            switch (outIndex) {
                case MediaCodec.INFO_OUTPUT_FORMAT_CHANGED:
                    break;
                case MediaCodec.INFO_TRY_AGAIN_LATER:
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

