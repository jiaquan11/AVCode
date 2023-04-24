#ifndef _FFDECODE_H_
#define _FFDECODE_H_

#include "XParameter.h"
#include "IDecode.h"

struct AVCodecContext;
struct AVFrame;

class FFDecode : public IDecode {
public:
    static void InitHard(void *vm);//静态函数

public:
    //打开解码器
    virtual bool Open(XParameter para, bool isHard = false);

    virtual void Clear();

    virtual void Close();

    //future模型 发送数据到线程解码
    virtual bool SendPacket(XData pkt);

    //从线程中获取解码结果，再次调用会复用上次空间，线程不安全
    virtual XData RecvFrame();

protected:
    AVCodecContext *codec = 0;
    AVFrame *frame = 0;
    std::mutex mux;

    unsigned char *datas[3] = {0};
};

#endif
