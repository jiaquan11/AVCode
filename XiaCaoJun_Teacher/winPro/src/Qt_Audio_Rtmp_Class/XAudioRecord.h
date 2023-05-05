#pragma once
#include <QThread>
enum XAUDIOTYPE {
	X_AUDIO_QT
};

struct XData {
	char* data = NULL;
	int size = 0;
	void Drop() {
		if (data) {
			delete data;
		}
		data = NULL;
		size = 0;
	}
};

class XAudioRecord : public QThread {
protected:
	XAudioRecord();

public:
	virtual ~XAudioRecord();

public:
	static XAudioRecord* Get(XAUDIOTYPE type = X_AUDIO_QT, int index = 0);//单例模式

	//纯虚函数，子类必须得实现
	//开始录制
	virtual bool Init() = 0;

	//停止录制
	virtual void Stop() = 0;

	//调用者清理空间
	virtual XData Pop() = 0;

public:
	int channels = 2;//声道数
	int sampleRate = 44100;//样本率
	int sampleByte = 2;//样本字节大小
	int nbSamples = 1024;//一帧音频每个通道的样本数量
};

