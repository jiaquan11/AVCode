package com.jiaquan.myplayer.listener;
public interface OnPcmInfoListener {
    void onPcmRate(int samplerate, int bit, int channels);
    void onPcmInfo(byte[] buffer, int buffersize);
}
