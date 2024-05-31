#ifndef MYPLAYER_WLPLAYSTATUS_H_
#define MYPLAYER_WLPLAYSTATUS_H_

/*
 * 记录播放状态
 * */
class WLPlayStatus {
public:
    WLPlayStatus();

    ~WLPlayStatus();

public:
    bool isExit = false;
    bool load = true;//默认是加载状态
    bool seek = false;
    bool pause = false;
};

#endif //MYPLAYER_WLPLAYSTATUS_H_
