#include <QtCore/QCoreApplication>
#include "XMediaEncode.h"
#include "XRtmp.h"
#include "XAudioRecord.h"

#include <iostream>
using namespace std;

int main(int argc, char *argv[]) {
	QCoreApplication a(argc, argv);

	//nginx-rtmp 直播服务器推流url
	char* outurl = "rtmp://192.168.141.128/myapp/";
	int ret = 0;

	int sampleRate = 44100;
	int channels = 2;
	int sampleByte = 2;
	int nbSamples = 1024;

	//1 qt音频开始录制
	XAudioRecord* ar = XAudioRecord::Get();
	ar->sampleRate = sampleRate;
	ar->channels = channels;
	ar->sampleByte = sampleByte;
	ar->nbSamples = nbSamples;
	if (!ar->Init()) {
		cout << "XAudioRecord Init failed" << endl;
		getchar();
		return -1;
	}

	XMediaEncode* xe = XMediaEncode::Get();
	xe->channels = channels;
	xe->nbSample = nbSamples;
	xe->sampleRate = sampleRate;
	xe->inSampleFmt = XSampleFMT::X_S16;
	xe->outSampleFmt = XSampleFMT::X_FLTP;
	if (!xe->InitResample()) {
		getchar();
		return -1;
	}

	//4 初始化音频编码器
	if (!xe->InitAudioCodec()) {
		getchar();
		return -1;
	}

	//5 输出封装器和音频流配置
	//a 创建输出封装器上下文
	XRtmp* xr = XRtmp::Get(0);
	if (!xr->Init(outurl)) {
		getchar();
		return -1;
	}
		
	//b 添加音频流
	if (!xr->AddStream(xe->ac)) {
		getchar();
		return -1;
	}

	//打开rtmp的网络输出IO
	//写入封装头
	if (!xr->SendHead()) {
		getchar();
		return -1;
	}

	for (;;) {
		//一次读取一帧音频
		XData d = ar->Pop();
		if (d.size <= 0) {
			QThread::msleep(1);
			continue;
		}

		//重采样源数据
		AVFrame* pcm = xe->Resample(d.data);
		d.Drop();

		//pts运算
		//nb_sample/sample_rate = 一帧音频的秒数sec
		//timebase pts = sec*timebase.den
		AVPacket *pkt = xe->EncodeAudio(pcm);
		if (!pkt) {
			continue;
		}
		//推流
		xr->SendFrame(pkt);
	}
	getchar();
	return a.exec();
}
