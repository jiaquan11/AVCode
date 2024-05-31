#ifndef MYPLAYER_WLBUFFERQUEUE_H_
#define MYPLAYER_WLBUFFERQUEUE_H_

#include <pthread.h>
#include <deque>

#include "log/android_log.h"
#include "wl_play_status.h"
#include "wl_pcm_bean.h"

/**
 * 用于存储PCM数据包的缓存队列类
 */
class WLBufferQueue {
public:
    WLBufferQueue(WLPlayStatus *play_status);

    ~WLBufferQueue();

public:
    int PutBuffer(SAMPLETYPE *buffer, int size);

    int GetBuffer(WLPcmBean **pcmbean);

    int GetBufferSize();

    int NoticeThread();

private:
    int _ClearBuffer();

private:
    std::deque<WLPcmBean *> m_queue_buffer_;
    pthread_mutex_t m_mutex_buffer_;
    pthread_cond_t m_cond_buffer_;
    WLPlayStatus *m_wlplay_status_ = NULL;
};

#endif //MYPLAYER_WLBUFFERQUEUE_H_
