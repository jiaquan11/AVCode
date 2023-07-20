#ifndef _WLQUEUE_H_
#define _WLQUEUE_H_

#include "log/androidLog.h"
#include "queue"
#include "pthread.h"
#include "WLPlayStatus.h"

extern "C" {
#include "libavcodec/avcodec.h"
};

/*
 * 存放packet的队列
 * */
class WLQueue {
public:
    WLQueue(WLPlayStatus *playStatus);

    ~WLQueue();

public:
    int putAVPacket(AVPacket *packet);

    int getAVPacket(AVPacket *packet);

    int getQueueSize();

    void clearAvPacket();

    void noticeQueue();

private:
    std::queue<AVPacket *> queuePacket;//包缓冲区队列
    pthread_mutex_t mutexPacket;
    pthread_cond_t condPacket;

    WLPlayStatus *playStatus = NULL;
};
#endif
