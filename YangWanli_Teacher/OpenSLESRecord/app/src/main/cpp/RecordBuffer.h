#ifndef _RECORDBUFFER_H_
#define _RECORDBUFFER_H_

class RecordBuffer {
public:
    RecordBuffer(int buffersize);

    ~RecordBuffer();

public:
    short *getRecordBuffer();

    short *getNowBuffer();

public:
    short **buffer;
    int index = -1;
};
#endif
