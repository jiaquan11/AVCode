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
    void PutBuffer(char* buffer, int size);

    void GetBuffer(WLPcmBean **pcm_bean);

    int GetBufferSize();

    void ClearBuffer();

    void NoticeQueue();

private:
    WLPlayStatus *m_play_status_ = NULL;
    std::deque<WLPcmBean *> m_buffer_queue_;
    pthread_mutex_t m_buffer_mutex_;
    pthread_cond_t m_buffer_cond_;

};

#endif //MYPLAYER_WLBUFFERQUEUE_H_
