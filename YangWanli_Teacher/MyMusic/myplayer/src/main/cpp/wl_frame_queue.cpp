#include "wl_frame_queue.h"

WLFrameQueue::WLFrameQueue(WLPlayStatus *play_status) {
    pthread_mutex_init(&m_frame_mutex_, NULL);
    pthread_cond_init(&m_frame_cond_, NULL);
    m_play_status_ = play_status;
}

WLFrameQueue::~WLFrameQueue() {
    ClearAVFrame();
    m_play_status_ = NULL;
    pthread_mutex_destroy(&m_frame_mutex_);
    pthread_cond_destroy(&m_frame_cond_);
}

void WLFrameQueue::PutAVFrame(AVFrame *frame) {
    pthread_mutex_lock(&m_frame_mutex_);
    m_frame_queue_.push(frame);
    NoticeQueue();//达到条件，发出信号，通知其它线程
    pthread_mutex_unlock(&m_frame_mutex_);
}

void WLFrameQueue::GetAVFrame(AVFrame *frame) {
    pthread_mutex_lock(&m_frame_mutex_);
    while ((m_play_status_ != NULL) && !m_play_status_->m_is_exit) {
        if (m_frame_queue_.size() > 0) {
            AVFrame *av_frame = m_frame_queue_.front();
            if (av_frame_ref(frame, av_frame) == 0) {
                m_frame_queue_.pop();
            }
            av_frame_free(&av_frame);
            break;
        } else {
            pthread_cond_wait(&m_frame_cond_, &m_frame_mutex_);//阻塞当前线程，其它线程可以继续操作
        }
    }
    pthread_mutex_unlock(&m_frame_mutex_);
}

int WLFrameQueue::GetQueueSize() {
    int size = 0;
    pthread_mutex_lock(&m_frame_mutex_);
    size = m_frame_queue_.size();
    pthread_mutex_unlock(&m_frame_mutex_);
    return size;
}

void WLFrameQueue::ClearAVFrame() {
    NoticeQueue();

    pthread_mutex_lock(&m_frame_mutex_);
    while (!m_frame_queue_.empty()) {
        AVFrame *frame = m_frame_queue_.front();
        m_frame_queue_.pop();
        av_frame_free(&frame);
    }
    pthread_mutex_unlock(&m_frame_mutex_);
}

void WLFrameQueue::NoticeQueue() {
    pthread_cond_signal(&m_frame_cond_);
}
