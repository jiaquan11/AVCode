#include "wl_packet_queue.h"

WLPacketQueue::WLPacketQueue(WLPlayStatus *play_status) {
    pthread_mutex_init(&m_packet_mutex_, NULL);
    pthread_cond_init(&m_packet_cond_, NULL);
    m_play_status_ = play_status;
}

WLPacketQueue::~WLPacketQueue() {
    ClearAVPacket();
    m_play_status_ = NULL;
    pthread_mutex_destroy(&m_packet_mutex_);
    pthread_cond_destroy(&m_packet_cond_);
}

void WLPacketQueue::PutAVPacket(AVPacket *packet) {
    pthread_mutex_lock(&m_packet_mutex_);
    m_packet_queue_.push(packet);
    NoticeQueue();//达到条件，发出信号，通知其它线程
    pthread_mutex_unlock(&m_packet_mutex_);
}

void WLPacketQueue::GetAVPacket(AVPacket *packet) {
    pthread_mutex_lock(&m_packet_mutex_);
    while ((m_play_status_ != NULL) && !m_play_status_->m_is_exit) {
        if (m_packet_queue_.size() > 0) {
            AVPacket *av_packet = m_packet_queue_.front();
            /**
             * av_packet_ref
             * 为packet分配内存并将src的内容拷贝到packet中
             */
            if (av_packet_ref(packet, av_packet) == 0) {
                m_packet_queue_.pop();
            }

            /**
             * av_packet_unref
             * 释放packet的引用，如果引用计数为0，释放数据buf
             * 仅释放 avPacket 内部的数据，不释放 avPacket 结构本身的内存
             * av_packet_free
             * 若avPacket 是动态分配的并且你需要释放整个结构和内部的数据，使用 av_packet_free(&avPacket).
             */
            av_packet_free(&av_packet);
            /**
             *不需要再调用 av_free(avPacket)，也不需要手动将 avPacket 置为 NULL，因为 av_packet_free 已经做了这一步。
             */
//            av_free(avPacket);
//            avPacket = NULL;
            break;
        } else {
            pthread_cond_wait(&m_packet_cond_, &m_packet_mutex_);//阻塞当前线程，其它线程可以继续操作
        }
    }
    pthread_mutex_unlock(&m_packet_mutex_);
}

int WLPacketQueue::GetQueueSize() {
    int size = 0;
    pthread_mutex_lock(&m_packet_mutex_);
    size = m_packet_queue_.size();
    pthread_mutex_unlock(&m_packet_mutex_);
    return size;
}

void WLPacketQueue::ClearAVPacket() {
    NoticeQueue();

    pthread_mutex_lock(&m_packet_mutex_);
    while (!m_packet_queue_.empty()) {
        AVPacket *packet = m_packet_queue_.front();
        m_packet_queue_.pop();
        av_packet_free(&packet);
    }
    pthread_mutex_unlock(&m_packet_mutex_);
}

void WLPacketQueue::NoticeQueue() {
    pthread_cond_signal(&m_packet_cond_);
}
