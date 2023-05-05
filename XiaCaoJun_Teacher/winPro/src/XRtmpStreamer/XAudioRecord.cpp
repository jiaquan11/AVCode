﻿#include "XAudioRecord.h"
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
		return true;
	}

	void Stop() {
		XDataThread::Stop();//先停止Qt音频采集线程

		if (input) {
			input->stop();//停止音频采集设备
			input = NULL;
		}
		if (io) {
			io->close();
			io = NULL;
		}
	}

	void run() {
		cout << "进入音频录制线程" << endl;
		//一次读取一帧音频的字节数
		int readSize = nbSamples * channels * sampleByte;
		char* buf = new char[readSize];
		while (!isExit) {
			//读取已录制音频
			//一次读取一帧音频
			if (input->bytesReady() < readSize) {
				QThread::msleep(1);
				continue;
			}

			int size = 0;
			while (size != readSize) {
				int len = io->read(buf + size, readSize - size);
				if (len < 0) break;
				size += len;
			}

			if (size != readSize) {
				continue;
			}

			//已经读取一帧音频
			long long pts = GetCurTime();//这里获取也是当前采集的音频pcm的采集时间戳
			Push(XData(buf, readSize, pts));
		}
		delete[]buf;
		cout << "退出音频录制线程" << endl;
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
