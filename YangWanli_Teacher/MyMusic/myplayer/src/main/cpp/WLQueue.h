#ifndef _WLQUEUE_H_
#define _WLQUEUE_H_

#include "log/androidLog.h"
#include "queue"
#include "pthread.h"
#include "WLPlayStatus.h"

extern "C" {
#include "libavcodec/avcodec.h"
};

class WLQueue {
public:
    WLQueue(WLPlayStatus *playStatus);

    ~WLQueue();

    int putAVPacket(AVPacket *packet);

    int getAVPacket(AVPacket *packet);

    int getQueueSize();

    void clearAvPacket();

    void noticeQueue();

public:
    std::queue<AVPacket *> queuePacket;
    pthread_mutex_t mutexPacket;
    pthread_cond_t condPacket;

    WLPlayStatus *playStatus = NULL;
};

#endif
