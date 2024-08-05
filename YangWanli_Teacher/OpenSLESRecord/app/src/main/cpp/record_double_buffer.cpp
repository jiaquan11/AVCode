#include "record_double_buffer.h"
#include "log/android_log.h"

RecordDoubleBuffer::RecordDoubleBuffer(int buffer_size, int num_buffers, bool &is_exit) : m_is_exit_(is_exit) {
    for (int i = 0; i < num_buffers; ++i) {
        short *buffer = new short[buffer_size];
        memset((char*)buffer, 0, buffer_size*sizeof(short));
        m_free_buffers_queue_.push(buffer);
    }
    m_buffer_size_ = buffer_size;
}

RecordDoubleBuffer::~RecordDoubleBuffer() {
    m_cond_free_.notify_one();
    while (!m_free_buffers_queue_.empty()) {
        delete[] m_free_buffers_queue_.front();
        m_free_buffers_queue_.pop();
    }
    m_cond_ready_.notify_one();
    while (!m_ready_buffers_queue_.empty()) {
        delete[] m_ready_buffers_queue_.front();
        m_ready_buffers_queue_.pop();
    }
}

short *RecordDoubleBuffer::GetWriteBuffer() {
    std::unique_lock<std::mutex> lock(m_mutex_);
    // 等待可用的写入缓冲区
    m_cond_free_.wait(lock, [this] { return !m_free_buffers_queue_.empty() || m_is_exit_; });
    if (m_is_exit_) {
        return NULL;
    }
    short *buffer = m_free_buffers_queue_.front();
    m_free_buffers_queue_.pop();
    if (buffer != NULL) {
        memset((char*)buffer, 0, m_buffer_size_* sizeof(short));
    }
    return buffer;
}

void RecordDoubleBuffer::CommitWriteBuffer(short *buffer) {
    {
        std::unique_lock<std::mutex> lock(m_mutex_);
        m_ready_buffers_queue_.push(buffer);
    }
    // 通知读取线程可用的数据
    m_cond_ready_.notify_one();
}

short *RecordDoubleBuffer::GetReadBuffer() {
    std::unique_lock<std::mutex> lock(m_mutex_);
    // 等待有准备好的读取缓冲区
    m_cond_ready_.wait(lock, [this] { return !m_ready_buffers_queue_.empty() || m_is_exit_; });
    if (m_is_exit_) {
        return NULL;
    }
    short *buffer = m_ready_buffers_queue_.front();
    if (buffer != NULL) {
        m_ready_buffers_queue_.pop();
    }
    return buffer;
}

void RecordDoubleBuffer::ReleaseReadBuffer(short *buffer) {
    {
        std::unique_lock<std::mutex> lock(m_mutex_);
        m_free_buffers_queue_.push(buffer);
    }
    // 通知写入线程可用的缓冲区
    m_cond_free_.notify_one();
}

void RecordDoubleBuffer::NotifyAll() {
    m_cond_free_.notify_all();
    m_cond_ready_.notify_all();
}

