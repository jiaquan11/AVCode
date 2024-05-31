#include "wl_buffer_queue.h"

WLBufferQueue::WLBufferQueue(WLPlayStatus *play_status) {
    m_wlplay_status_ = play_status;
    pthread_mutex_init(&m_mutex_buffer_, NULL);
    pthread_cond_init(&m_cond_buffer_, NULL);
}

WLBufferQueue::~WLBufferQueue() {
    m_wlplay_status_ = NULL;
    _ClearBuffer();
    pthread_mutex_destroy(&m_mutex_buffer_);
    pthread_cond_destroy(&m_cond_buffer_);
    if (LOG_DEBUG) {
        LOGI("WLBufferQueue 释放完成");
    }
}

int WLBufferQueue::PutBuffer(SAMPLETYPE *buffer, int size) {
    pthread_mutex_lock(&m_mutex_buffer_);
    WLPcmBean *pcmBean = new WLPcmBean(buffer, size);
    m_queue_buffer_.push_back(pcmBean);
    pthread_cond_signal(&m_cond_buffer_);
    pthread_mutex_unlock(&m_mutex_buffer_);
    return 0;
}

int WLBufferQueue::GetBuffer(WLPcmBean **pcmBean) {
    pthread_mutex_lock(&m_mutex_buffer_);
    while ((m_wlplay_status_ != NULL) && !m_wlplay_status_->m_is_exit) {
        if (m_queue_buffer_.size() > 0) {
            *pcmBean = m_queue_buffer_.front();
            m_queue_buffer_.pop_front();
            break;
        } else {
            if (!m_wlplay_status_->m_is_exit) {
                pthread_cond_wait(&m_cond_buffer_, &m_mutex_buffer_);//阻塞当前线程，唤醒其它线程
            }
        }
    }
    pthread_mutex_unlock(&m_mutex_buffer_);
    return 0;
}

int WLBufferQueue::GetBufferSize() {
    int size = 0;
    pthread_mutex_lock(&m_mutex_buffer_);
    size = m_queue_buffer_.size();
    pthread_mutex_unlock(&m_mutex_buffer_);
    return size;
}

int WLBufferQueue::NoticeThread() {
    pthread_cond_signal(&m_cond_buffer_);
    return 0;
}

int WLBufferQueue::_ClearBuffer() {
    NoticeThread();
    pthread_mutex_lock(&m_mutex_buffer_);
    while (!m_queue_buffer_.empty()) {
        WLPcmBean *pcmBean = m_queue_buffer_.front();
        m_queue_buffer_.pop_front();
        delete pcmBean;
    }
    pthread_mutex_unlock(&m_mutex_buffer_);
    return 0;
}
