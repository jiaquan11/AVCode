#include "XRtmp.h"
#include <iostream>
#include <string>
using namespace std;

extern "C" {
#include <libavformat/avformat.h>
}

class CXRtmp : public XRtmp
{
private:
	//rtmp flv封装器
	AVFormatContext* oc = NULL;
	string url = "";
	const AVCodecContext* vc = NULL;//视频编码器流
	AVStream* vs = NULL;

public:
	void Close() {
		if (oc) {
			avformat_close_input(&oc);
			vs = NULL;
		}
		vc = NULL;
		url = "";
	}

	bool Init(const char* url) {
		//输出封装器和视频流配置
		//创建输出封装器上下文
		this->url = url;

		int ret = avformat_alloc_output_context2(&oc, 0, "flv", url);
		if (ret != 0) {
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			cout << buf << endl;
			return false;
		}
		return true;
	}

	//添加视频或音频流
	bool AddStream(const AVCodecContext* c) {
		if (!c) return false;

		//添加视频流
		AVStream* st = avformat_new_stream(oc, NULL);
		if (!st) {
			cout << "avformat_new_stream failed!" << endl;
			return false;
		}
		st->codecpar->codec_tag = 0;
		//从编码器复制参数
		avcodec_parameters_from_context(st->codecpar, c);

		av_dump_format(oc, 0, url.c_str(), 1);

		if (c->codec_type == AVMEDIA_TYPE_VIDEO) {
			vc = c;
			vs = st;
		}

		return true;
	}

	bool SendHead() {
		//打开rtmp的网络输出IO
		int ret = avio_open(&oc->pb, url.c_str(), AVIO_FLAG_WRITE);
		if (ret != 0) {
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			cout << buf << endl;
			return false;
		}

		//写入封装头
		ret = avformat_write_header(oc, NULL);
		if (ret != 0) {
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			cout << buf << endl;
			return false;
		}
		return true;
	}

	bool SendFrame(AVPacket* packet) {
		if ((packet->size) <= 0 || !packet->data) {
			return false;
		}

		//推流
		packet->pts = av_rescale_q(packet->pts, vc->time_base, vs->time_base);
		packet->dts = av_rescale_q(packet->dts, vc->time_base, vs->time_base);
		packet->duration = av_rescale_q(packet->duration, vc->time_base, vs->time_base);
		packet->pos = -1;
		int ret = av_interleaved_write_frame(oc, packet);
		return true;
	}
};

XRtmp::XRtmp() {

}

XRtmp::~XRtmp() {

}

//工厂生产方法
XRtmp* XRtmp::Get(unsigned char index) {
	static CXRtmp cxr[255];

	static bool isFirst = true;
	if (isFirst) {
		//注册所有的封装器
		//av_register_all();
		//注册所有网络协议
		avformat_network_init();
		isFirst = false;
	}
	return &cxr[index];
}
