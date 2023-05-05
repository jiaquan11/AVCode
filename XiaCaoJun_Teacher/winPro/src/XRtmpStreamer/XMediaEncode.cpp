#include "XMediaEncode.h"
#include <iostream>
using namespace std;

extern "C" {
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
}

#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "swresample.lib")

#if defined WIN32 || defined _WIN32
#include <windows.h>
#endif
//获取CPU数量
static int XGetCpuNum() {
#if defined WIN32 || defined _WIN32
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);

	return (int)sysinfo.dwNumberOfProcessors;
#elif defined __linux__
	return (int)sysconf(_SC_NPROCESSORS_ONLN);
#elif defined __APPLE__
	int numCPU = 0;
	int mib[4];
	size_t len = sizeof(numCPU);

	// set the mib for hw.ncpu
	mib[0] = CTL_HW;
	mib[1] = HW_AVAILCPU;  // alternatively, try HW_NCPU;

						   // get the number of CPUs from the system
	sysctl(mib, 2, &numCPU, &len, NULL, 0);

	if (numCPU < 1)
	{
		mib[1] = HW_NCPU;
		sysctl(mib, 2, &numCPU, &len, NULL, 0);

		if (numCPU < 1)
			numCPU = 1;
	}
	return (int)numCPU;
#else
	return 1;
#endif
}

class CXMediaEncode : public XMediaEncode {
private:
	SwsContext* vsc = NULL;//像素格式转换上下文
	SwrContext* asc = NULL;//音频重采样上下文
	AVFrame* yuv = NULL;//输出的YUV
	AVFrame* pcm = NULL;//重采样输出的pcm
	AVPacket vpacket = {0};//视频帧
	AVPacket apacket = {0};//音频帧
	int vpts = 0;
	int apts = 0;

private:
	AVCodecContext* CreateCodec(AVCodecID cid) {
		//4 初始化编码器
		AVCodec* codec = avcodec_find_encoder(cid);
		if (!codec) {
			cout << "avcodec_find_encoder failed!" << endl;
			return NULL;
		}
		//音频编码器上下文
		AVCodecContext* c = avcodec_alloc_context3(codec);
		if (!c) {
			cout << "avcodec_alloc_context3 failed" << endl;
			return NULL;
		}
		cout << "avcodec_alloc_context3 success!" << endl;

		c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		c->thread_count = XGetCpuNum();
		c->time_base = { 1, 1000000 };//统一设置编码器的时间基准为1/1000000
		return c;
	}
	
	bool OpenCodec(AVCodecContext** c) {
		//打开音频编码器
		int ret = avcodec_open2(*c, 0, 0);
		if (ret != 0) {
			char err[1024] = { 0 };
			av_strerror(ret, err, sizeof(err) - 1);
			cout << err << endl;
			avcodec_free_context(c);
			return false;
		}
		cout << "avcodec_open2 success!" << endl;
		return true;
	}

public:
	void Close() {
		if (vsc) {
			sws_freeContext(vsc);
			vsc = NULL;
		}
		if (asc) {
			swr_free(&asc);
		}
		if (vc) {
			avcodec_free_context(&vc);
		}
		if (ac) {
			avcodec_free_context(&ac);
		}
		if (yuv) {
			av_frame_free(&yuv);
		}
		if (pcm) {
			av_frame_free(&pcm);
		}

		vpts = 0;
		apts = 0;
		av_packet_unref(&vpacket);
		av_packet_unref(&apacket);
	}

	bool InitScale() {
		//初始化格式转换上下文
		vsc = sws_getCachedContext(vsc,
			inWidth, inHeight, AV_PIX_FMT_BGR24,//原宽高，像素格式
			outWidth, outHeight, AV_PIX_FMT_YUV420P,//目标宽高，像素格式
			SWS_BICUBIC,//尺寸变化使用算法
			0, 0, 0);
		if (!vsc) {
			cout<<"sws_getCachedContext failed!";
			return false;
		}

		//初始化输出的数据结构
		yuv = av_frame_alloc();
		yuv->format = AV_PIX_FMT_YUV420P;
		yuv->width = inWidth;
		yuv->height = inHeight;
		yuv->pts = 0;
		//分配yuv内存
		int ret = av_frame_get_buffer(yuv, 32);
		if (ret != 0) {
			cout << "av_frame_get_buffer failed!";
			return false;
		}
		return true;
	}

	XData RGBToYUV(XData d) {
		XData r;
		//rgb to yuv
		//输入的数据结构
		uint8_t *indata[AV_NUM_DATA_POINTERS] = { 0 };
		//indata[0] bgrbgrbgr
		//plane indata[0]bbbbb indata[1]ggggg indata[2]rrrrr
		indata[0] = (uint8_t*)d.data;
		int insize[AV_NUM_DATA_POINTERS] = { 0 };
		//一行(宽)数据的字节数
		insize[0] = inWidth*inPixSize;

		int h = sws_scale(vsc, indata, insize, 0, inHeight, //原数据
			yuv->data, yuv->linesize);
		if (h <= 0) {
			return r;
		}

		r.pts = d.pts;
		yuv->pts = d.pts;
		r.data = (char*)yuv;
		//实际就是累加得到一帧yuv的大小
		r.size = yuv->linesize[0] * outHeight + yuv->linesize[1] * outHeight / 2 + yuv->linesize[2] * outHeight / 2;//yuv大小
		return r;
	}

	bool InitResample() {
		//2音频重采样，上下文初始化
		asc = swr_alloc_set_opts(asc,
			av_get_default_channel_layout(channels), (AVSampleFormat)outSampleFmt, sampleRate,//输出格式
			av_get_default_channel_layout(channels), (AVSampleFormat)inSampleFmt, sampleRate, 0, 0);//输入格式
		if (!asc) {
			cout << "swr_alloc_set_opts failed!" << endl;
			return false;
		}
		int ret = swr_init(asc);
		if (ret != 0) {
			char err[1024] = { 0 };
			av_strerror(ret, err, sizeof(err) - 1);
			cout << err << endl;
			return false;
		}
		cout << "音频重采样上下文初始化成功！" << endl;

		//3 音频重采样输出空间分配
		pcm = av_frame_alloc();
		pcm->format = outSampleFmt;
		pcm->channels = channels;
		pcm->channel_layout = av_get_default_channel_layout(channels);
		pcm->nb_samples = nbSamples;//一帧音频一通道的采样数量
		ret = av_frame_get_buffer(pcm, 0);//给pcm分配存储空间
		if (ret != 0) {
			char err[1024] = { 0 };
			av_strerror(ret, err, sizeof(err) - 1);
			cout << err << endl;
			return false;
		}
		return true;
	}

	XData Resample(XData d) {
		XData r;
		//重采样源数据
		const uint8_t* indata[AV_NUM_DATA_POINTERS] = { 0 };
		indata[0] = (uint8_t*)d.data;
		int len = swr_convert(asc, pcm->data, pcm->nb_samples,//输出参数，输出存储地址
			indata, pcm->nb_samples);
		if (len <= 0) {
			return r;
		}

		pcm->pts = d.pts;
		r.data = (char*)pcm;
		r.size = pcm->nb_samples*pcm->channels * av_get_bytes_per_sample((AVSampleFormat)outSampleFmt);
		r.pts = d.pts;
		return r;
	}

	bool InitVideoCodec() {
		if (!(vc = CreateCodec(AV_CODEC_ID_H264))) {
			return false;
		}

		vc->bit_rate = 50 * 1024 * 8;//压缩后每秒视频的bit位大小 50KB
		vc->width = outWidth;
		vc->height = outHeight;
		//vc->time_base = { 1, fps };
		vc->framerate = { fps, 1 };
		//画面组的大小，多少帧一个关键帧
		vc->gop_size = 50;
		vc->max_b_frames = 0;
		vc->pix_fmt = AV_PIX_FMT_YUV420P;
		return OpenCodec(&vc);
	}

	bool InitAudioCodec() {
		if (!(ac = CreateCodec(AV_CODEC_ID_AAC))) {
			return false;
		}

		ac->bit_rate = 40000;
		ac->sample_rate = sampleRate;
		ac->sample_fmt = (AVSampleFormat)X_FLTP;
		ac->channels = channels;
		ac->channel_layout = av_get_default_channel_layout(channels);
		return OpenCodec(&ac);
	}

	XData EncodeVideo(XData frame) {
		XData r;
		if ((frame.size <= 0) || !frame.data) {
			return r;
		}
		AVFrame* p = (AVFrame*)frame.data;

		//h264编码
	/*	frame->pts = vpts;
		vpts++;*/
		int ret = avcodec_send_frame(vc, p);
		if (ret != 0) {
			return r;
		}

		av_packet_unref(&vpacket);//要注意packet的内部buf的释放
		ret = avcodec_receive_packet(vc, &vpacket);
		if ((ret != 0) || (vpacket.size <= 0)) {
			return r;
		}

		r.data = (char*)&vpacket;
		r.size = vpacket.size;
		r.pts = frame.pts;
		return r;
	}

	long long lasta = -1;
	XData EncodeAudio(XData frame) {
		XData r;
		if ((frame.size <= 0) || !frame.data) {
			return r;
		}
		AVFrame* p = (AVFrame*)frame.data;
		if (lasta == p->pts) {//对于前后音频数据计时时间戳可能一致的处理
			p->pts += 1000;
		}
		lasta = p->pts;

		int ret = avcodec_send_frame(ac, p);
		if (ret != 0) {
			return r;
		}

		av_packet_unref(&apacket);//要注意packet的内部buf的释放
		ret = avcodec_receive_packet(ac, &apacket);
		if (ret != 0) {
			return r;
		}

		r.data = (char*)&apacket;
		r.size = apacket.size;
		r.pts = frame.pts;
		return r;
	}
};

XMediaEncode::XMediaEncode() {

}

XMediaEncode::~XMediaEncode() {

}

XMediaEncode* XMediaEncode::Get(unsigned char index) {
	static bool isFirst = true;
	if (isFirst) {
		//注册所有的编码器
		avcodec_register_all();
		isFirst = false;
	}
	static CXMediaEncode cxm[255];
	return &cxm[index];
}