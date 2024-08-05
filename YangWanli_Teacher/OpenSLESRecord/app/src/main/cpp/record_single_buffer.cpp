#include "record_single_buffer.h"
#include "log/android_log.h"

RecordSingleBuffer::RecordSingleBuffer(int buffer_size) {
    m_buffer_ = new short[buffer_size];
}

RecordSingleBuffer::~RecordSingleBuffer() {
    delete [] m_buffer_;
}

short *RecordSingleBuffer::GetRecordBuffer() {
    return m_buffer_;
}
