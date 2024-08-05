#ifndef RECORD_DOUBLE_BUFFER_H_
#define RECORD_DOUBLE_BUFFER_H_

#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>

/**
 * 音频录制缓冲区
 * 双缓冲区设计
 */
class RecordDoubleBuffer {
public:
    RecordDoubleBuffer(int buffer_size, int num_buffers, bool &is_exit);

    ~RecordDoubleBuffer();

public:
    short *GetWriteBuffer();

    void CommitWriteBuffer(short *buffer);

    short *GetReadBuffer();

    void ReleaseReadBuffer(short *buffer);

    void NotifyAll();

private:
    std::queue<short *> m_free_buffers_queue_;   // 可用的写缓冲区队列
    std::queue<short *> m_ready_buffers_queue_;  // 准备好的读缓冲区队列
    std::mutex m_mutex_;
    std::condition_variable m_cond_free_;  // 用于通知写缓冲区的可用性
    std::condition_variable m_cond_ready_; // 用于通知读缓冲区的可用性
    bool &m_is_exit_;
    int m_buffer_size_ = 0;
};

#endif //RECORD_DOUBLE_BUFFER_H_
