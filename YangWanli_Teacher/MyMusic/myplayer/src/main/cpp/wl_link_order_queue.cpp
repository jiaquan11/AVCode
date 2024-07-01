#include "wl_link_order_queue.h"

WLLinkOrderQueue::WLLinkOrderQueue()
        : m_head_(NULL),
          m_size_(0) {
    pthread_mutex_init(&m_pts_mutex_, NULL);
}

/**
 * 析构函数，释放链表内部所有结点内存
 */
WLLinkOrderQueue::~WLLinkOrderQueue() {
    _Flush();
    pthread_mutex_destroy(&m_pts_mutex_);
}

/**
 * 插入新节点，插入前会进行与队列中的数据对比排序
 * @param data
 */
void WLLinkOrderQueue::Push(int data) {
    LinkOrderQueueNode *node = new LinkOrderQueueNode();
    pthread_mutex_lock(&m_pts_mutex_);
    node->data = data;
    node->next = NULL;
    m_size_++;
    if (m_head_ == NULL) {
        m_head_ = node;//头节点存放实际数据
    } else {
        if (node->data <= m_head_->data) {
            /**
             * 如果新插入的节点数据比头结点的数据小或者相等，则需要插入头结点的前面，并将头指针指向新节点位置,
             * 保证头指针指向第一个节点的位置
             */
            node->next = m_head_;
            m_head_ = node;
        } else {
            /**
             * 如果新插入的节点数据比头结点数据大，则需要依次与头结点后面各个节点数据对比大小排序，
             * 直到遇到比此节点数据大的就停止
             */
            LinkOrderQueueNode *p = m_head_->next;
            LinkOrderQueueNode *q = m_head_;
            while ((p != NULL) && (p->data < node->data)) {
                q = p;
                p = p->next;
            }
            node->next = p;//将新节点插入q节点和p节点的中间
            q->next = node;
        }
    }
    pthread_mutex_unlock(&m_pts_mutex_);
}

/**
 * 弹出头结点,返回结点中的数据，并销毁结点，弹出节点会从队列头依次出队(已排好序)
 * @return int
 */
int WLLinkOrderQueue::Popup() {
    int data = 0;
    LinkOrderQueueNode *node = NULL;
    pthread_mutex_lock(&m_pts_mutex_);
    if (m_head_ == NULL) {
        data = -1;
    } else {
        node = m_head_;
        m_head_ = m_head_->next;
        if (node != NULL) {
            data = node->data;
            delete node;
            node = NULL;
            m_size_--;
            if (m_size_ < 0) {
                m_size_ = 0;
            }
        }
    }
    pthread_mutex_unlock(&m_pts_mutex_);
    return data;
}

/**
 * 获取队列大小
 * @return int 队列大小
 */
int WLLinkOrderQueue::Size() {
    int size = 0;
    pthread_mutex_lock(&m_pts_mutex_);
    size = _Size();
    pthread_mutex_unlock(&m_pts_mutex_);
    return size;
}

/**
 * 弹出头结点,并销毁
 */
void WLLinkOrderQueue::_Popup() {
    if (m_head_ == NULL) {
        return;
    }else{
        LinkOrderQueueNode *node = m_head_;
        m_head_ = m_head_->next;
        delete node;
        node = NULL;
        m_size_--;
        if (m_size_ < 0) {
            m_size_ = 0;
        }
    }
}

/**
 * 清空所有结点
 */
void WLLinkOrderQueue::_Flush() {
    pthread_mutex_lock(&m_pts_mutex_);
    int size = _Size();
    while (size > 0) {
        _Popup();
        size--;
    }
    m_size_ = 0;
    m_head_ = NULL;
    pthread_mutex_unlock(&m_pts_mutex_);
}

int WLLinkOrderQueue::_Size() {
    return m_size_;
}


