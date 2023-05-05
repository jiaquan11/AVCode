#pragma once
#include "XDataThread.h"

enum XAUDIOTYPE {
	X_AUDIO_QT //QT音频采集
};

class XAudioRecord : public XDataThread {
protected:
	XAudioRecord();

public:
	virtual ~XAudioRecord();

public:
	static XAudioRecord* Get(XAUDIOTYPE type = X_AUDIO_QT, int index = 0);

	//开始录制
	virtual bool Init() = 0;

	//停止录制
	virtual void Stop() = 0;

public:
	int channels = 2;//声道数
	int sampleRate = 44100;//样本率
	int sampleByte = 2;//样本字节大小
	int nbSamples = 1024;//一帧音频每个通道的样本数量
};

