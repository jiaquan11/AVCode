#pragma once
#include <QThread>
#include <QMutex>
#include "XData.h"

//线程类
class XDataThread : public QThread {
public:
	XDataThread();

	virtual ~XDataThread();

public:
	//启动线程
	virtual bool Start();

	//退出线程,并等待线程退出(阻塞)
	virtual void Stop();

	//在列表结尾插入
	virtual void Push(XData d);

	//读取列表中最早的数据,返回数据需要调用XData.Drop清理
	virtual XData Pop();

	//清理列表
	virtual void Clear();

protected:
	//存放交互数据,插入策略，先进先出
	std::list<XData> datas;

	//缓冲区列表大小，链表最大值，超出删除最旧的数据
	int maxList = 100;

	//交互数据列表大小
	int dataCount = 0;
	//互斥访问 datas
	QMutex mutex;
	//处理线程退出
	bool isExit = false;
};

