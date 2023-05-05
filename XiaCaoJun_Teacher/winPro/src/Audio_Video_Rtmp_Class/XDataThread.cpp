#include "XDataThread.h"

XDataThread::XDataThread(){

}

XDataThread::~XDataThread(){

}

//启动线程
bool XDataThread::Start() {
	isExit = false;
	QThread::start();//启动线程
	return true;
}

//退出线程,并等待线程退出(阻塞)
void XDataThread::Stop() {
	isExit = true;
	wait();
}

//在列表结尾插入
void XDataThread::Push(XData d) {
	mutex.lock();
	//大于缓冲最大值时，从队头开始丢帧
	if (datas.size() > maxList) {
		datas.front().Drop();
		datas.pop_front();
	}
	datas.push_back(d);//插入缓冲队尾
	mutex.unlock();
}

//读取列表中最早的数据
XData XDataThread::Pop() {
	mutex.lock();
	if (datas.empty()) {
		mutex.unlock();
		return XData();
	}
	XData d = datas.front();
	datas.pop_front();
	mutex.unlock();
	return d;
}

//清空缓存队列中的数据
void XDataThread::Clear() {
	mutex.lock();
	while (!datas.empty()) {
		datas.front().Drop();
		datas.pop_front();
	}
	mutex.unlock();
}