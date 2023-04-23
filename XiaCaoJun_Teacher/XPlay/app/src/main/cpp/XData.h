#ifndef _XDATA_H_
#define _XDATA_H_

//数据类型
enum XDataType {
    AVPACKET_TYPE = 0,
    UCHAR_TYPE = 1
};

struct XData {
    int type = 0;//默认是AVPACKET_TYPE类型
    int pts = 0;//packet pts
    unsigned char *data = 0;
    unsigned char *datas[8] = {0};
    int size = 0;
    bool isAudio = false;
    int width = 0;
    int height = 0;
    int format = 0;

    bool Alloc(int size, const char *data = 0);

    void Drop();
};

#endif
