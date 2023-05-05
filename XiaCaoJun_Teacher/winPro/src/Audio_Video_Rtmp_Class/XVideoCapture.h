#pragma once
#include "XDataThread.h"
#include "XFilter.h"
#include <vector>
#include <QMutex>

class XVideoCapture : public XDataThread {
protected:
	XVideoCapture();

public:
	virtual ~XVideoCapture();

public:
	static XVideoCapture* Get(unsigned char index = 0);

	virtual bool Init(int camIndex = 0) = 0;//打开本地摄像头

	virtual bool Init(const char *url) = 0;//打开rtsp网络摄像头，传入摄像头采集地址

	virtual void Stop() = 0;

	//添加滤镜
	void AddFilter(XFilter* f) {
		fmutex.lock();
		filters.push_back(f);
		fmutex.unlock();
	}

protected:
	QMutex fmutex;
	std::vector<XFilter*> filters;//滤镜容器，用于存放多个滤镜

public:
	int width = 0;
	int height = 0;
	int fps = 0;
};

