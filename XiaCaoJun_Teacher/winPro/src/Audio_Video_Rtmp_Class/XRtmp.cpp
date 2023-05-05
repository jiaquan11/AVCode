#include "XRtmp.h"
#include <iostream>
#include <string>
using namespace std;

extern "C" {
	#include <libavformat/avformat.h>
}

#pragma comment(lib, "avformat.lib")

class CXRtmp : public XRtmp {
private:
	//rtmp flv封装器
	AVFormatContext* oc = NULL;
	string url = "";
	const AVCodecContext* vc = NULL;//视频编码器流
	AVStream* vs = NULL;//视频流
	const AVCodecContext* ac = NULL;//音频编码器流
	AVStream* as = NULL;//音频流

public:
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
	int AddStream(const AVCodecContext* c) {
		if (!c) return -1;

		//添加视频流
		AVStream *st = avformat_new_stream(oc, NULL);
		if (!st) {
			cout << "avformat_new_stream failed!" << endl;
			return -1;
		}
		st->codecpar->codec_tag = 0;
		//从编码器复制参数
		avcodec_parameters_from_context(st->codecpar, c);

		av_dump_format(oc, 0, url.c_str(), 1);

		if (c->codec_type == AVMEDIA_TYPE_VIDEO) {
			vc = c;
			vs = st;
		}
		else if (c->codec_type == AVMEDIA_TYPE_AUDIO) {
			ac = c;
			as = st;
		}
		return st->index;
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

	bool SendFrame(XData d, int streamIndex) {
		if (!d.data || (d.size <= 0)) {
			return false;
		}

		AVPacket* packet = (AVPacket*)d.data;
		packet->stream_index = streamIndex;

		AVRational stime;
		AVRational dtime;
		if (vs && vc && (packet->stream_index == vs->index)) {
			stime = vc->time_base;
			dtime = vs->time_base;
		} else if (as && ac && (packet->stream_index == as->index)) {
			stime = ac->time_base;
			dtime = as->time_base;
		} else {
			return false;
		}

		//推流  时间基准需要从编码器的时间基准转换为流的时间基准
		packet->pts = av_rescale_q(packet->pts, stime, dtime);
		packet->dts = av_rescale_q(packet->dts, stime, dtime);
		packet->duration = av_rescale_q(packet->duration, stime, dtime);
		packet->pos = -1;
		int ret = av_interleaved_write_frame(oc, packet);
		if (ret == 0) {
			cout << "#" << flush;
			return true;
		}
		return false;
	}

	void Close() {
		if (oc) {
			avformat_close_input(&oc);
			vs = NULL;
		}
		vc = NULL;
		url = "";
	}
};

XRtmp::XRtmp(){

}

XRtmp::~XRtmp(){

}

//工厂生产方法
XRtmp* XRtmp::Get(unsigned char index) {
	static CXRtmp cxr[255];
	static bool isFirst = true;
	if (isFirst) {
		//注册所有的封装器
		av_register_all();
		//注册所有网络协议
		avformat_network_init();
		isFirst = false;
	}
	return &cxr[index];
}
