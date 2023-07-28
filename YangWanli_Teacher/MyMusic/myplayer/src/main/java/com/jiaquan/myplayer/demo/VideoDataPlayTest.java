package com.jiaquan.myplayer.demo;

import android.media.MediaCodec;
import android.media.MediaFormat;
import android.util.Log;
import android.view.Surface;

import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;

public class VideoDataPlayTest extends Thread {
    private final String TAG = VideoDataPlayTest.class.getSimpleName();

    private MediaCodec decoder = null;
    private long totalTime = 0;
    private int frameCount = 0;
    private Surface surface = null;
    private MediaFormat mVideoFormat = null;
    private String mMine = "video/hevc";
    private int mWidth = 1080;
    private int mHeight = 1920;
    private byte[] bytes = null;
    private int mAvgTime = 0;

    public VideoDataPlayTest() {

    }

    public void setSurface(Surface s) {
        surface = s;
    }

    public int getWidth() {
        return mWidth;
    }

    public int getHeight() {
        return mHeight;
    }

    @Override
    public void run() {
        try {
            bytes = getBytes("/sdcard/testziliao/hanleiVideo.265");
        } catch (IOException e) {
            e.printStackTrace();
        }
        Log.i(TAG, "bytes size " + bytes.length);

        try {
            decoder = MediaCodec.createDecoderByType(mMine);
        } catch (IOException e) {
            e.printStackTrace();
        }

        if (decoder == null) {
            return;
        }

        mVideoFormat = MediaFormat.createVideoFormat(mMine, mWidth, mHeight);
        mVideoFormat.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, mWidth * mHeight);
        decoder.configure(mVideoFormat, surface, null, 0);
        decoder.start();

        //开始位置
        int startIndex = 0;
        //h265总字节数
        int totalSize = bytes.length;

        MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();
        long startMs = System.currentTimeMillis();
        boolean isEOS = false;
        while (!Thread.interrupted()) {
            //判断是否符合
            if ((totalSize == 0) || (startIndex >= totalSize)) {
                break;
            }

            /*寻找索引
             这里只会寻找00 00 00 01然后进行读取，也就是送给解码器的可能是VPS，SPS，PPS及一帧实际的码流图像
             分别单独的送入解码器，这样也是可以正常解码并渲染。
             也就是并不需要在配置解码器之前进行单独设置VPS SPS PPS
                mediaFormat.setByteBuffer("csd-0", ByteBuffer.wrap(csd));
                mediaFormat.setByteBuffer("csd-1", ByteBuffer.wrap(csd));
                if (mime.equals("video/hevc")) {
                    mediaFormat.setByteBuffer("csd-2", ByteBuffer.wrap(csd));
                }
             */
            int nextFrameStart = findByFrame(bytes, startIndex + 1, totalSize);
            Log.i(TAG, "nextFrameStart " + nextFrameStart);

            if (!isEOS) {
                int inIndex = decoder.dequeueInputBuffer(10000);
                if (inIndex >= 0) {
                    ByteBuffer buffer = null;
                    if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.LOLLIPOP) {
                        buffer = decoder.getInputBuffer(inIndex);
                        buffer.clear();
                    }

                    if (nextFrameStart == -1) {
                        decoder.queueInputBuffer(inIndex, 0, 0, 0, MediaCodec.BUFFER_FLAG_END_OF_STREAM);
                        isEOS = true;
                    } else {
                        buffer.put(bytes, startIndex, nextFrameStart - startIndex);
                        decoder.queueInputBuffer(inIndex, 0, nextFrameStart - startIndex, 0, 0);
                    }
                    //为下一帧做准备，下一帧首就是前一帧的尾。
                    startIndex = nextFrameStart;
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
        mAvgTime = (int)(totalTime / frameCount);
        Log.i(TAG, "All the Frames: " + frameCount + ", Average decode time per frame: " + mAvgTime + "ms");
    }

    //读取一帧数据
    private int findByFrame(byte[] bytes, int start, int totalSize) {
        for (int i = start; i < totalSize - 4; i++) {
            //对output.h265文件分析 可通过分隔符 0x00000001 读取真正的数据
            if ((bytes[i] == 0x00) && (bytes[i + 1] == 0x00) && (bytes[i + 2] == 0x00) && (bytes[i + 3] == 0x01)) {
                return i;
            }
        }
        return -1;
    }

    //一次性读取文件内容到输出流中
    private byte[] getBytes(String videoPath) throws IOException {
        InputStream is = new DataInputStream(new FileInputStream(new File(videoPath)));
        int len;
        int size = 1024;
        byte[] buf;
        ByteArrayOutputStream bos = new ByteArrayOutputStream();
        buf = new byte[size];
        while ((len = is.read(buf, 0, size)) != -1) {//循环读取
            bos.write(buf, 0, len);
        }
        buf = bos.toByteArray();
        return buf;
    }
}
