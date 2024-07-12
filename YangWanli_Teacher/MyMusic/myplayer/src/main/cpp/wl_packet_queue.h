#ifndef MYPLAYER_WLPACKET_QUEUE_H_
#define MYPLAYER_WLPACKET_QUEUE_H_

#include "pthread.h"
#include "queue"
extern "C" {
    #include "libavcodec/avcodec.h"
}

#include "log/android_log.h"
#include "wl_play_status.h"

/**
 * 音视频Packet缓冲区队列
 */
class WLPacketQueue {
public:
    WLPacketQueue(WLPlayStatus *play_status);

    ~WLPacketQueue();

public:
    void PutAVPacket(AVPacket *packet);

    void GetAVPacket(AVPacket *packet);

    int GetQueueSize();

    void ClearAVPacket();

    void NoticeQueue();

private:
    WLPlayStatus *m_play_status_ = NULL;
    std::queue<AVPacket *> m_packet_queue_;
    pthread_mutex_t m_packet_mutex_;
    pthread_cond_t m_packet_cond_;
};

#endif //MYPLAYER_WLPACKET_QUEUE_H_
