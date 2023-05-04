#include "XMediaEncode.h"
#include <iostream>
using namespace std;

extern "C" {
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#if defined WIN32 || defined _WIN32
#include <windows.h>
#endif
//��ȡCPU����
static int XGetCpuNum()
{
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

class CXMediaEncode : public XMediaEncode
{
private:
	SwsContext* vsc = NULL;//���ظ�ʽת��������
	AVFrame* yuv = NULL;//�����YUV
	AVPacket packet = { 0 };
	int vpts = 0;

public:
	void Close() {
		if (vsc) {
			sws_freeContext(vsc);
			vsc = NULL;
		}
		if (yuv) {
			av_frame_free(&yuv);
		}
		if (vc) {
			avcodec_free_context(&vc);
		}
		vpts = 0;
		av_packet_unref(&packet);
	}

	bool InitVideoCodec() {
		//��ʼ������������
		//�ҵ�������
		AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
		if (!codec) {
			cout << "Can't find h264 encoder!" << endl;
			return false;
		}

		//����������������
		vc = avcodec_alloc_context3(codec);
		if (!vc) {
			cout << "avcodec_alloc_context3 failed!" << endl;;
		}
		//���ñ���������
		vc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;//ȫ�ֲ���
		vc->codec_id = codec->id;
		vc->thread_count = XGetCpuNum();//��ȡ�߳���
		vc->bit_rate = 50 * 1024 * 8;//ѹ����ÿ����Ƶ��bitλ��С 50KB
		vc->width = outWidth;
		vc->height = outHeight;
		vc->time_base = { 1, fps };
		vc->framerate = { fps, 1 };
		//������Ĵ�С������֡һ���ؼ�֡
		vc->gop_size = 60;
		vc->max_b_frames = 0;
		vc->pix_fmt = AV_PIX_FMT_YUV420P;
		//�򿪱�����������
		int ret = avcodec_open2(vc, 0, 0);
		if (ret != 0) {
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			cout << buf << endl;
			return false;
		}
		cout << "avcodec_open2 success!" << endl;
		return true;
	}

	AVPacket* EncodeVideo(AVFrame* frame) {
		av_packet_unref(&packet);

		//h264����
		frame->pts = vpts;
		vpts++;
		int ret = avcodec_send_frame(vc, frame);
		if (ret != 0) {
			return NULL;
		}

		ret = avcodec_receive_packet(vc, &packet);
		if ((ret != 0) || (packet.size <= 0)) {
			return NULL;
		}
		return &packet;
	}

	bool InitScale() {
		//��ʼ����ʽת��������
		vsc = sws_getCachedContext(vsc,
			inWidth, inHeight, AV_PIX_FMT_BGR24,//ԭ��ߣ����ظ�ʽ
			outWidth, outHeight, AV_PIX_FMT_YUV420P,//Ŀ���ߣ����ظ�ʽ
			SWS_BICUBIC,//�ߴ�仯ʹ���㷨
			0, 0, 0);
		if (!vsc) {
			cout << "sws_getCachedContext failed!";
			return false;
		}

		//��ʼ����������ݽṹ
		yuv = av_frame_alloc();
		yuv->format = AV_PIX_FMT_YUV420P;
		yuv->width = inWidth;
		yuv->height = inHeight;
		yuv->pts = 0;
		//����yuv�ڴ�
		int ret = av_frame_get_buffer(yuv, 32);
		if (ret != 0) {
			cout << "av_frame_get_buffer failed!";
			return false;
		}
		return true;
	}

	AVFrame* RGBToYUV(char* rgb) {
		//rgb to yuv
		//��������ݽṹ
		uint8_t* indata[AV_NUM_DATA_POINTERS] = { 0 };
		//indata[0] bgrbgrbgr
		//plane indata[0]bbbbb indata[1]ggggg indata[2]rrrrr
		indata[0] = (uint8_t*)rgb;
		int insize[AV_NUM_DATA_POINTERS] = { 0 };
		//һ��(��)���ݵ��ֽ���
		insize[0] = inWidth * inPixSize;

		int h = sws_scale(vsc, indata, insize, 0, inHeight, //ԭ����
			yuv->data, yuv->linesize);
		if (h <= 0) {
			return NULL;
		}
		return yuv;
	}
};

XMediaEncode::XMediaEncode() {

}

XMediaEncode::~XMediaEncode() {

}

XMediaEncode* XMediaEncode::Get(unsigned char index) {
	static bool isFirst = true;
	if (isFirst) {
		//ע�����еı�����
		//avcodec_register_all();
		isFirst = false;
	}

	static CXMediaEncode cxm[255];
	return &cxm[index];
}