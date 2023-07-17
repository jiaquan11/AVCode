#include <string.h>
#include "RecordBuffer.h"

//分配两片相同的数组内存
RecordBuffer::RecordBuffer(int buffersize) {
    buffer = new short *[2];
    for (int i = 0; i < 2; ++i) {
        buffer[i] = new short[buffersize];
    }
}

RecordBuffer::~RecordBuffer() {
    for (int i = 0; i < 2; ++i) {
        delete buffer[i];
    }
    delete buffer;
}

//切换到下一个索引的数组内存
short *RecordBuffer::getRecordBuffer() {
    index++;
    if (index > 1) {
        index = 1;
    }
    return buffer[index];
}

//获取当前索引的数组内存
short *RecordBuffer::getNowBuffer() {
    return buffer[index];
}


