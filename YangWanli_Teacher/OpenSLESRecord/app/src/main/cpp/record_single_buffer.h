#ifndef RECORD_SINGLE_BUFFER_H_
#define RECORD_SINGLE_BUFFER_H_

#include <stdlib.h>
/**
 * 音频录制缓冲区(单个缓冲区)
 */
class RecordSingleBuffer {
public:
    RecordSingleBuffer(int buffer_size);

    ~RecordSingleBuffer();

public:
    short* GetRecordBuffer();

private:
    short* m_buffer_ = NULL;
};

#endif //RECORD_SINGLE_BUFFER_H_
