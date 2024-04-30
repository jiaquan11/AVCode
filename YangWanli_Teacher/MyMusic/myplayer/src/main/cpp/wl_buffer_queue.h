#ifndef MYPLAYER_WLBUFFERQUEUE_H_
#define MYPLAYER_WLBUFFERQUEUE_H_

#include "log/androidLog.h"
#include <deque>
#include "wl_play_status.h"
#include "wl_pcm_bean.h"
#include <pthread.h>

/**
 * 用于存储PCM数据包的缓存队列类
 */
class WLBufferQueue {
public:
    WLBufferQueue(WLPlayStatus *playStatus);

    ~WLBufferQueue();

public:
    int putBuffer(SAMPLETYPE *buffer, int size);

    int getBuffer(WLPcmBean **pcmBean);

    int getBufferSize();

    int noticeThread();

private:
    int clearBuffer();

private:
    std::deque<WLPcmBean *> queueBuffer;
    pthread_mutex_t mutexBuffer;
    pthread_cond_t condBuffer;
    WLPlayStatus *wlPlayStatus = NULL;
};

#endif //MYPLAYER_WLBUFFERQUEUE_H_
