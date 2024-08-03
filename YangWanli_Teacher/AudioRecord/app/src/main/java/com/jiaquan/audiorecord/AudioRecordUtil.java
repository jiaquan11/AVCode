package com.jiaquan.audiorecord;

import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.util.Log;

public class AudioRecordUtil {
    private AudioRecord mAudioRecord_ = null;
    private int mBufferSizeInBytes_ = 0;
    private boolean mStart_ = false;
    private int mReadSize_ = 0;

    private OnRecordListener mOnRecordListener_ = null;
    public interface OnRecordListener {
        void recordByte(byte[] audioData, int readSize);
    }
    public void setOnRecordListener(OnRecordListener onRecordListener) {
        mOnRecordListener_ = onRecordListener;
    }

    public AudioRecordUtil() {
        mBufferSizeInBytes_ = AudioRecord.getMinBufferSize(44100, AudioFormat.CHANNEL_IN_STEREO, AudioFormat.ENCODING_PCM_16BIT);
        Log.i("AudioRecordUtil", "mBufferSizeInBytes_: " + mBufferSizeInBytes_);
        mAudioRecord_ = new AudioRecord(MediaRecorder.AudioSource.MIC, 44100,
                                        AudioFormat.CHANNEL_IN_STEREO, AudioFormat.ENCODING_PCM_16BIT, mBufferSizeInBytes_);
    }

    public void startRecord() {
        new Thread() {
            @Override
            public void run() {
                mStart_ = true;
                mAudioRecord_.startRecording();
                byte[] audioData = new byte[mBufferSizeInBytes_];
                while (mStart_) {
                    mReadSize_ = mAudioRecord_.read(audioData, 0, mBufferSizeInBytes_);
                    if (mOnRecordListener_ != null) {
                        mOnRecordListener_.recordByte(audioData, mReadSize_);
                    }
                }
                if (mAudioRecord_ != null) {
                    mAudioRecord_.stop();
                    mAudioRecord_.release();
                    mAudioRecord_ = null;
                }
            }
        }.start();
    }

    public void stopRecord() {
        mStart_ = false;
    }
}
