#pragma once
struct AVFrame;
struct AVPacket;
struct AVCodecContext;

//音视频编码接口类
class XMediaEncode {
protected:
	XMediaEncode();

public:
	virtual ~XMediaEncode();

public:
	//工厂生产方法
	static XMediaEncode* Get(unsigned char index = 0);

	//纯虚函数，子类必须实现方法
	//视频编码器初始化
	virtual bool InitVideoCodec() = 0;

	//视频编码
	virtual AVPacket* EncodeVideo(AVFrame* frame) = 0;

	//初始化像素格式转换的上下文初始化
	virtual bool InitScale() = 0;

	virtual AVFrame* RGBToYUV(char* rgb) = 0;

public:
	AVCodecContext* vc = 0;//编码器上下文

	//输入参数
	int inWidth = 1280;
	int inHeight = 720;
	int inPixSize = 3;

	//输出参数
	int outWidth = 1280;
	int outHeight = 720;
	int bitrate = 4000000;//压缩后每秒视频的bit位大小
	int fps = 30;
};

