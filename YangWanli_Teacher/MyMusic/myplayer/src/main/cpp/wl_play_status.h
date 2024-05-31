#ifndef MYPLAYER_WLPLAYSTATUS_H_
#define MYPLAYER_WLPLAYSTATUS_H_

/**
 * 播放状态
 */
class WLPlayStatus {
public:
    WLPlayStatus();

    ~WLPlayStatus();

public:
    bool m_is_exit = false;
    bool m_load = true;
    bool m_seek = false;
    bool m_pause = false;
};

#endif //MYPLAYER_WLPLAYSTATUS_H_
