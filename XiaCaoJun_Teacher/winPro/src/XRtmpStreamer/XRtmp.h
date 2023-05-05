﻿#pragma once
#include "XData.h"
class AVCodecContext;
class AVPacket;

class XRtmp {
protected:
	XRtmp();

public:
	virtual ~XRtmp();

public:
	//工厂生产方法
	static XRtmp* Get(unsigned char index = 0);

	//初始化封装器上下文
	virtual bool Init(const char* url) = 0;

	//添加视频或音频流,失败返回-1,成功返回流索引
	virtual int AddStream(const AVCodecContext* c) = 0;

	//打开rtmp网络IO,发送封装头
	virtual bool SendHead() = 0; 

	//rtmp帧推流
	virtual bool SendFrame(XData d, int streamIndex = 0) = 0;

	virtual void Close() = 0;
};

