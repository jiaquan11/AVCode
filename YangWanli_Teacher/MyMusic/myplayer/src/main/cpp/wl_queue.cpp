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

void WLQueue::PutAVPacket(AVPacket *packet) {
    pthread_mutex_lock(&m_mutex_packet_);
    m_queue_packet_.push(packet);
    _NoticeQueue();//达到条件，发出信号，通知其它线程
    pthread_mutex_unlock(&m_mutex_packet_);
}

void WLQueue::GetAVPacket(AVPacket *packet) {
    pthread_mutex_lock(&m_mutex_packet_);
    while ((m_play_status_ != NULL) && !m_play_status_->m_is_exit) {
        if (m_queue_packet_.size() > 0) {
            AVPacket *avPacket = m_queue_packet_.front();
            /**
             * av_packet_ref
             * 为packet分配内存并将src的内容拷贝到packet中
             */
            if (av_packet_ref(packet, avPacket) == 0) {
                m_queue_packet_.pop();
            }

            /**
             * av_packet_unref
             * 释放packet的引用，如果引用计数为0，释放数据buf
             * 仅释放 avPacket 内部的数据，不释放 avPacket 结构本身的内存
             * av_packet_free
             * 若avPacket 是动态分配的并且你需要释放整个结构和内部的数据，使用 av_packet_free(&avPacket).
             */
            av_packet_free(&avPacket);
            /**
             *不需要再调用 av_free(avPacket)，也不需要手动将 avPacket 置为 NULL，因为 av_packet_free 已经做了这一步。
             */
//            av_free(avPacket);
//            avPacket = NULL;
            break;
        } else {
            pthread_cond_wait(&m_cond_packet_, &m_mutex_packet_);//阻塞当前线程，其它线程可以继续操作
        }
    }
    pthread_mutex_unlock(&m_mutex_packet_);
}

int WLQueue::GetQueueSize() {
    int size = 0;
    pthread_mutex_lock(&m_mutex_packet_);
    size = m_queue_packet_.size();
    pthread_mutex_unlock(&m_mutex_packet_);
    return size;
}

void WLQueue::ClearAvPacket() {
    _NoticeQueue();//直接通知唤醒，不需要加锁

    pthread_mutex_lock(&m_mutex_packet_);
    while (!m_queue_packet_.empty()) {
        AVPacket *packet = m_queue_packet_.front();
        m_queue_packet_.pop();
        av_packet_free(&packet);
    }
    pthread_mutex_unlock(&m_mutex_packet_);
}

void WLQueue::_NoticeQueue() {
    pthread_cond_signal(&m_cond_packet_);
}
