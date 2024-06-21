package com.jiaquan.myplayer.listener;
public interface OnPcmInfoListener {
    void onPcmInfo(int samplerate, int bit, int channels);
    void onPcmData(byte[] buffer, int buffersize);
}
