#include "wl_buffer_queue.h"

WLBufferQueue::WLBufferQueue(WLPlayStatus *play_status) {
    m_play_status_ = play_status;
    pthread_mutex_init(&m_buffer_mutex_, NULL);
    pthread_cond_init(&m_buffer_cond_, NULL);
}

WLBufferQueue::~WLBufferQueue() {
    ClearBuffer();
    m_play_status_ = NULL;
    pthread_mutex_destroy(&m_buffer_mutex_);
    pthread_cond_destroy(&m_buffer_cond_);
}

void WLBufferQueue::PutBuffer(char* buffer, int size) {
    pthread_mutex_lock(&m_buffer_mutex_);
    WLPcmBean *pcm_bean = new WLPcmBean(buffer, size);
    m_buffer_queue_.push_back(pcm_bean);
    NoticeQueue();
    pthread_mutex_unlock(&m_buffer_mutex_);
}

void WLBufferQueue::GetBuffer(WLPcmBean **pcm_bean) {
    pthread_mutex_lock(&m_buffer_mutex_);
    while ((m_play_status_ != NULL) && !m_play_status_->m_is_exit) {
        if (m_buffer_queue_.size() > 0) {
            *pcm_bean = m_buffer_queue_.front();
            m_buffer_queue_.pop_front();
            break;
        } else {
            pthread_cond_wait(&m_buffer_cond_, &m_buffer_mutex_);//阻塞当前线程，唤醒其它线程
        }
    }
    pthread_mutex_unlock(&m_buffer_mutex_);
}

int WLBufferQueue::GetBufferSize() {
    int size = 0;
    pthread_mutex_lock(&m_buffer_mutex_);
    size = m_buffer_queue_.size();
    pthread_mutex_unlock(&m_buffer_mutex_);
    return size;
}

void WLBufferQueue::ClearBuffer() {
    NoticeQueue();

    pthread_mutex_lock(&m_buffer_mutex_);
    while (!m_buffer_queue_.empty()) {
        WLPcmBean *pcm_bean = m_buffer_queue_.front();
        m_buffer_queue_.pop_front();
        delete pcm_bean;
    }
    pthread_mutex_unlock(&m_buffer_mutex_);
}

void WLBufferQueue::NoticeQueue() {
    pthread_cond_signal(&m_buffer_cond_);
}
