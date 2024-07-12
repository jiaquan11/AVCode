#ifndef MYPLAYER_WLFRAME_QUEUE_H_
#define MYPLAYER_WLFRAME_QUEUE_H_

#include "pthread.h"
#include "queue"
extern "C" {
    #include "libavcodec/avcodec.h"
}

#include "log/android_log.h"
#include "wl_play_status.h"

/**
 * 音视频Frame缓冲区队列
 */
class WLFrameQueue {
public:
    WLFrameQueue(WLPlayStatus *play_status);

    ~WLFrameQueue();

public:
    void PutAVFrame(AVFrame *avframe);

    void GetAVFrame(AVFrame *avframe);

    int GetQueueSize();

    void ClearAVFrame();

    void NoticeQueue();

private:
    WLPlayStatus *m_play_status_ = NULL;
    std::queue<AVFrame *> m_frame_queue_;
    pthread_mutex_t m_frame_mutex_;
    pthread_cond_t m_frame_cond_;
};

#endif //MYPLAYER_WLFRAME_QUEUE_H_
