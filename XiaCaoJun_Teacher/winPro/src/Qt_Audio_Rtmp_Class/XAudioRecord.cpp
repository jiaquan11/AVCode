#include "XAudioRecord.h"
#include <QAudioInput>
#include <QAudioFormat>
#include <iostream>
#include <QMutex>
#include <list>

using namespace std;

class CXAudioRecord : public XAudioRecord {
public:
	QAudioInput* input = NULL;
	QIODevice* io = NULL;

	bool isExit = false;//控制线程操作
	QMutex mutex;
	list<XData> datas;
	int maxList = 100;

public:
	bool Init() {
		Stop();

		//1.qt音频开始录制
		QAudioFormat fmt;
		fmt.setSampleRate(sampleRate);
		fmt.setChannelCount(channels);
		fmt.setSampleSize(sampleByte * 8);
		fmt.setCodec("audio/pcm");
		fmt.setByteOrder(QAudioFormat::LittleEndian);
		fmt.setSampleType(QAudioFormat::UnSignedInt);
		QAudioDeviceInfo info = QAudioDeviceInfo::defaultInputDevice();
		if (!info.isFormatSupported(fmt)) {
			cout << "Audio format not support!" << endl;
			fmt = info.nearestFormat(fmt);
		}

		input = new QAudioInput(fmt);
		//开始录制音频
		io = input->start();
		if (!io) {
			return false;
		}

		QThread::start();//开启Qt音频录制
		isExit = false;
		return true;
	}

	void Stop() {
		isExit = true;
		wait();

		if (input) {
			input->stop();
			input = NULL;
		}
		if (io) {
			io->close();
			io = NULL;
		}
	}

	//获取缓冲队列中已录制采集的音频数据
	XData Pop() {
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

	void run() {
		cout << "进入音频录制线程" << endl;
		//一次读取一帧音频的字节数
		int readSize = nbSamples*channels*sampleByte;
		while (!isExit){
			//读取已录制音频
			//一次读取一帧音频
			if (input->bytesReady() < readSize) {
				QThread::msleep(1);
				continue;
			}

			char* buf = new char[readSize];
			int size = 0;
			while (size != readSize) {
				int len = io->read(buf + size, readSize - size);
				if (len < 0) break;
				size += len;
			}

			if (size != readSize) {
				delete []buf;
				continue;
			}

			//已经读取一帧音频
			XData d;
			d.data = buf;
			d.size = readSize;
			mutex.lock();
			if (datas.size() > maxList) {//超过缓冲队列最大值，则丢包
				datas.front().Drop();
				datas.pop_front();
			}
			datas.push_back(d);
			mutex.unlock();
		}
	}
};

XAudioRecord::XAudioRecord(){

}

XAudioRecord::~XAudioRecord(){

}

XAudioRecord* XAudioRecord::Get(XAUDIOTYPE type, int index) {
	static CXAudioRecord record[255];
	return &record[index];
}
