#ifndef MYPLAYER_WLQUEUE_H_
#define MYPLAYER_WLQUEUE_H_

#include "log/android_log.h"
#include "queue"
#include "pthread.h"
#include "wl_play_status.h"

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

#endif //MYPLAYER_WLQUEUE_H_
