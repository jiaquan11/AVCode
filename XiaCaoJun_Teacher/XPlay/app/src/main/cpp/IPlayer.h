#ifndef _IPLAYER_H_
#define _IPLAYER_H_

#include <mutex>
#include "XThread.h"
#include "XParameter.h"

class IDemux;

class IAudioPlay;

class IVideoView;

class IResample;

class IDecode;

class IPlayer : public XThread {
public:
    static IPlayer *Get(unsigned char index = 0);

    virtual void InitView(void *win);

    virtual bool Open(const char *path);

    virtual void Close();

    virtual bool Start();

    virtual bool Seek(double pos);

    virtual void SetPause(bool isP);

    //获取当前的播放进度 0.0-1.0
    virtual double PlayPos();

public:
    IDemux *demux = 0;
    IDecode *vdecode = 0;
    IDecode *adecode = 0;
    IResample *resample = 0;
    IVideoView *videoView = 0;
    IAudioPlay *audioPlay = 0;

    //音频输出参数配置
    XParameter outPara;
    //是否视频硬解
    bool isHardDecode = true;

protected:
    IPlayer() {};

    //用作音视频同步
    void Main();

protected:
    std::mutex mux;
};

#endif
