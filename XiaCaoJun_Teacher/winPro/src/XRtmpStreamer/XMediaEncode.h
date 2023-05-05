#pragma once
#include "XData.h"
struct AVCodecContext;

enum XSampleFMT {
	X_S16 = 1,
	X_FLTP = 8
};

//音视频编码接口类
class XMediaEncode {
protected:
	XMediaEncode();

public:
	virtual ~XMediaEncode();

public:
	AVCodecContext* vc = 0;//视频编码器上下文
	AVCodecContext* ac = 0;//音频编码器上下文

	//输入参数
	int inWidth = 1280;
	int inHeight = 720;
	int inPixSize = 3;
	int channels = 2;
	int sampleRate = 44100;
	XSampleFMT inSampleFmt = X_S16;

	//输出参数
	int outWidth = 1280;
	int outHeight = 720;
	int bitrate = 4000000;//压缩后每秒视频的bit位大小
	int fps = 30;
	XSampleFMT outSampleFmt = X_FLTP;
	int nbSamples = 1024;

public:
	//工厂生产方法
	static XMediaEncode* Get(unsigned char index = 0);

	//初始化像素格式转换的上下文初始化
	virtual bool InitScale() = 0;

	//音频重采样上下文初始化
	virtual bool InitResample() = 0;

	//返回值无需调用者清理
	virtual XData RGBToYUV(XData d) = 0;

	//返回值无需调用者清理
	virtual XData Resample(XData d) = 0;

	//视频编码器初始化
	virtual bool InitVideoCodec() = 0;

	//视频编码器初始化
	virtual bool InitAudioCodec() = 0;

	//视频编码 返回值无需调用者清理
	virtual XData EncodeVideo(XData frame) = 0;

	//音频编码 返回值无需调用者清理
	virtual XData EncodeAudio(XData frame) = 0;

	virtual void Close() = 0;
};

