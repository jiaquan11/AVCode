#ifndef _WLBUFFERQUEUE_H_
#define _WLBUFFERQUEUE_H_

#include "log/androidLog.h"
#include <deque>
#include "WLPlayStatus.h"
#include "WLPcmBean.h"
#include <pthread.h>

class WLBufferQueue {
public:
    WLBufferQueue(WLPlayStatus *playStatus);

    ~WLBufferQueue();

    int putBuffer(SAMPLETYPE *buffer, int size);

    int getBuffer(WLPcmBean **pcmBean);

    int clearBuffer();

    int getBufferSize();

    void release();

    int noticeThread();

public:
    std::deque<WLPcmBean *> queueBuffer;
    pthread_mutex_t mutexBuffer;
    pthread_cond_t condBuffer;
    WLPlayStatus *wlPlayStatus = NULL;
};
#endif
