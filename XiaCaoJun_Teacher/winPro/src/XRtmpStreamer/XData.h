#pragma once
class XData {
public:
	XData();
	//创建空间，并复制data内容
	XData(char* data, int size, long long p = 0);
	virtual ~XData();

public:
	void Drop();

public:
	char* data = 0;
	int size = 0;
	long long pts = 0;//记录当前数据的时间戳
};

//获取当前时间戳(微秒)
long long GetCurTime();