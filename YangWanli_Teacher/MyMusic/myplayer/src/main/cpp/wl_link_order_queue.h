#ifndef MYPLAYER_LINK_ORDER_QUEUE_H_
#define MYPLAYER_LINK_ORDER_QUEUE_H_

#include "pthread.h"
#include "log/android_log.h"

/**
 * 排序队列，用于将时间戳进行排序
 * 插入即做排序
 */
struct LinkOrderQueueNode {
    int data;
    LinkOrderQueueNode *next;
};

class WLLinkOrderQueue {
public:
    WLLinkOrderQueue();

    ~WLLinkOrderQueue();

public:
    void Push(int data);

    int Popup();

    void Clear();

    int Size();

private:
    void _Popup();

    int _Size();

private:
    LinkOrderQueueNode *m_head_;

    pthread_mutex_t m_pts_mutex_;

    int m_size_;
};

#endif

