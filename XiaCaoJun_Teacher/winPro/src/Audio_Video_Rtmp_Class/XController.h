#pragma once
#include "XDataThread.h"

class XController : public XDataThread {
protected:
	XController();

public:
	virtual ~XController();

public:
	static XController *Get() {//����ģʽ
		static XController xc;
		return &xc;
	}

	//�趨���ղ���
	virtual bool Set(std::string key, double value);

	virtual bool Start();

	virtual void Stop();

	void run();

protected:
	int vindex = 0;//��Ƶ��������
	int aindex = 1;//��Ƶ��������

public:
	std::string outUrl;
	int camIndex = -1;
	std::string inUrl = "";
	std::string err = "";
};

