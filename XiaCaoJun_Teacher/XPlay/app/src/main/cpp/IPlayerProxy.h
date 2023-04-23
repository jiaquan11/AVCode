#ifndef _IPLAYERPROXY_H_
#define _IPLAYERPROXY_H_

#include <mutex>
#include "IPlayer.h"

//播放器代理类，单例模式
class IPlayerProxy : public IPlayer {
public:
    static IPlayerProxy *Get() {//单例模式
        static IPlayerProxy px;
        return &px;
    }

    void Init(void *vm = 0);

    virtual void InitView(void *win);

    virtual bool Open(const char *path);

    virtual void Close();

    virtual bool Start();

    virtual bool Seek(double pos);

    virtual void SetPause(bool isP);

    virtual bool IsPause();

    //获取当前的播放进度 0.0-1.0
    virtual double PlayPos();

protected:
    IPlayerProxy() {}

protected:
    IPlayer *player = 0;
    std::mutex mux;
};

#endif
