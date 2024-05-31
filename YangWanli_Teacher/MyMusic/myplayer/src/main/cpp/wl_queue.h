#ifndef MYPLAYER_WLQUEUE_H_
#define MYPLAYER_WLQUEUE_H_

#include "pthread.h"
#include "queue"
extern "C" {
    #include "libavcodec/avcodec.h"
}

#include "log/android_log.h"
#include "wl_play_status.h"

/**
 * 音视频包缓冲区队列
 */
class WLQueue {
public:
    WLQueue(WLPlayStatus *playStatus);

    ~WLQueue();

public:
    int PutAVPacket(AVPacket *packet);

    int GetAVPacket(AVPacket *packet);

    int GetQueueSize();

    void NoticeQueue();

    void ClearAvPacket();

private:
    std::queue<AVPacket *> m_queue_packet_;//包缓冲区队列
    pthread_mutex_t m_mutex_packet_;
    pthread_cond_t m_cond_packet_;
    WLPlayStatus *m_play_status_ = NULL;
};

#endif //MYPLAYER_WLQUEUE_H_
