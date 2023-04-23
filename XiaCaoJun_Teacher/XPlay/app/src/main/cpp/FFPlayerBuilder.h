#ifndef _FFPLAYERBUILDER_H_
#define _FFPLAYERBUILDER_H_

#include "IPlayerBuilder.h"

//构建FFmpeg播放器类，也是单例模式
class FFPlayerBuilder : public IPlayerBuilder {
public:
    static void InitHard(void *vm);//静态函数

    static FFPlayerBuilder *Get() {//单例模式
        static FFPlayerBuilder ff;
        return &ff;
    }

protected:
    FFPlayerBuilder() {}

protected:
    virtual IPlayer *CreatePlayer(unsigned char index = 0);

    virtual IDemux *CreateDemux();

    virtual IDecode *CreateDecode();

    virtual IResample *CreateResample();

    virtual IVideoView *CreateVideoView();

    virtual IAudioPlay *CreateAudioPlay();
};

#endif
