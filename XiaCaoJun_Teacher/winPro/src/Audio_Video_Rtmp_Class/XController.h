#pragma once
#include "XDataThread.h"

class XController : public XDataThread {
protected:
	XController();

public:
	virtual ~XController();

public:
	static XController *Get() {//单例模式
		static XController xc;
		return &xc;
	}

	//设定美颜参数
	virtual bool Set(std::string key, double value);

	virtual bool Start();

	virtual void Stop();

	void run();

protected:
	int vindex = 0;//视频流索引号
	int aindex = 1;//音频流索引号

public:
	std::string outUrl;
	int camIndex = -1;
	std::string inUrl = "";
	std::string err = "";
};

