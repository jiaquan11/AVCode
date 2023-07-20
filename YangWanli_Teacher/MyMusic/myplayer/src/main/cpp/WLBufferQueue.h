#ifndef _WLBUFFERQUEUE_H_
#define _WLBUFFERQUEUE_H_

#include "log/androidLog.h"
#include <deque>
#include "WLPlayStatus.h"
#include "WLPcmBean.h"
#include <pthread.h>

/*
 * 存储PCM数据包的缓存队列类
 * */
class WLBufferQueue {
public:
    WLBufferQueue(WLPlayStatus *playStatus);

    ~WLBufferQueue();

public:
    int putBuffer(SAMPLETYPE *buffer, int size);

    int getBuffer(WLPcmBean **pcmBean);

    int getBufferSize();

    void release();

    int noticeThread();

private:
    int clearBuffer();

private:
    std::deque<WLPcmBean *> queueBuffer;
    pthread_mutex_t mutexBuffer;
    pthread_cond_t condBuffer;
    WLPlayStatus *wlPlayStatus = NULL;
};
#endif
