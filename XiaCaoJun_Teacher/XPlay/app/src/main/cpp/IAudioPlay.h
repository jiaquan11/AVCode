#ifndef _IAUDIOPLAY_H_
#define _IAUDIOPLAY_H_

#include <list>
#include "IObserver.h"
#include "XParameter.h"

class IAudioPlay : public IObserver {
public:
    virtual bool StartPlay(XParameter out) = 0;

    virtual void Clear();

    virtual void Close() = 0;

    //获取缓冲数据，如没有则阻塞
    virtual XData GetData();

    //缓冲满后阻塞
    virtual void Update(XData data);

public:
    //最大缓冲
    int maxFrames = 100;
    int pts = 0;

protected:
    std::list<XData> frames;
    std::mutex framesMutex;
};

#endif
