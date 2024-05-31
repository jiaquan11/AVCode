#include "wl_queue.h"

WLQueue::WLQueue(WLPlayStatus *play_status) {
    pthread_mutex_init(&m_mutex_packet_, NULL);
    pthread_cond_init(&m_cond_packet_, NULL);
    m_play_status_ = play_status;
}

WLQueue::~WLQueue() {
    ClearAvPacket();

    pthread_mutex_destroy(&m_mutex_packet_);
    pthread_cond_destroy(&m_cond_packet_);
}

int WLQueue::PutAVPacket(AVPacket *packet) {
    pthread_mutex_lock(&m_mutex_packet_);
    m_queue_packet_.push(packet);
    if (LOG_DEBUG) {
//        LOGI("put a packet into queue, the count: %d", m_queue_packet_.size());
    }

    NoticeQueue();
    pthread_mutex_unlock(&m_mutex_packet_);
    return 0;
}

int WLQueue::GetAVPacket(AVPacket *packet) {
    pthread_mutex_lock(&m_mutex_packet_);
    while ((m_play_status_ != NULL) && !m_play_status_->m_is_exit) {
        if (m_queue_packet_.size() > 0) {
            AVPacket *avPacket = m_queue_packet_.front();
            /*
             * av_packet_ref将原packet的buf引用赋值给目标packet,建立一个新的引用
             * 其它字段全部进行拷贝
             * */
            if (av_packet_ref(packet, avPacket) == 0) {
                m_queue_packet_.pop();
            }
            /*
             * av_packet_free
             * avPacket如果有引用计数，av_packet_free会取消引用，但不会释放数据buf。
             * 只有没有引用计数为0，才会释放数据buf, 释放packet malloc的内存
             * */
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            if (LOG_DEBUG) {
//                LOGI("get a packet from the queue, the rest: %d", m_queue_packet_.size());
            }
            break;
        } else {
            pthread_cond_wait(&m_cond_packet_, &m_mutex_packet_);//阻塞当前线程，其它线程可以继续操作
        }
    }
    pthread_mutex_unlock(&m_mutex_packet_);
    return 0;
}

int WLQueue::GetQueueSize() {
    int size = 0;
    pthread_mutex_lock(&m_mutex_packet_);
    size = m_queue_packet_.size();
    pthread_mutex_unlock(&m_mutex_packet_);
    return size;
}

//清除缓冲区队列
void WLQueue::ClearAvPacket() {
    NoticeQueue();

    pthread_mutex_lock(&m_mutex_packet_);
    while (!m_queue_packet_.empty()) {
        AVPacket *packet = m_queue_packet_.front();
        m_queue_packet_.pop();
        av_packet_free(&packet);
        av_free(packet);
        packet = NULL;
    }
    pthread_mutex_unlock(&m_mutex_packet_);
}

void WLQueue::NoticeQueue() {
    pthread_cond_signal(&m_cond_packet_);
}
