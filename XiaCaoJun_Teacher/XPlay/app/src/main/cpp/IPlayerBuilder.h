#ifndef _IPLAYERBUILDER_H_
#define _IPLAYERBUILDER_H_

#include "IPlayer.h"

//播放器构建的基类
class IPlayerBuilder {
public:
    virtual IPlayer *BuilderPlayer(unsigned char index = 0);

protected://下面都为纯虚函数，子类继承后必须得实现
    virtual IPlayer *CreatePlayer(unsigned char index = 0) = 0;

    virtual IDemux *CreateDemux() = 0;

    virtual IDecode *CreateDecode() = 0;

    virtual IResample *CreateResample() = 0;

    virtual IVideoView *CreateVideoView() = 0;

    virtual IAudioPlay *CreateAudioPlay() = 0;
};

#endif
