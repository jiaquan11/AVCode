#include <opencv2/highgui.hpp>
#include <iostream>

extern "C" {
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

using namespace std;
using namespace cv;

int main(int argc, char* argv[]) {
	//ע�����еķ�װ��
	//av_register_all();
	//ע�����еı������
	//avcodec_register_all();
	//ע����������Э��
	avformat_network_init();
	//���������rtsp url
	const char* inUrl = "rtsp://test:test123456@192.168.1.109";
	//nginx-rtmp ֱ������������url
	const char* outurl = "rtmp://192.168.141.128/myapp/";

	VideoCapture cam;
	Mat frame;
	namedWindow("video");

	//���ظ�ʽת��������
	SwsContext* vsc = NULL;
	//��������ݽṹ
	AVFrame* yuv = NULL;
	//������������
	AVCodecContext* vc = NULL;
	//rtmp flv��װ��
	AVFormatContext* oc = NULL;

	try {
		/////////////////////////////////////////////
		///1 ʹ��opencv��rtsp���
		//cam.open(inUrl);
		cam.open(0);
		if (!cam.isOpened()) {
			throw exception("cam open failed!");
		}
		cout << inUrl << "cam open success!" << endl;
		int inWidth = cam.get(CAP_PROP_FRAME_WIDTH);
		int inHeight = cam.get(CAP_PROP_FRAME_HEIGHT);
		int fps = cam.get(CAP_PROP_FPS);//�����ȡ������ͷ֡�ʿ���Ϊ0���ᵼ�º����Ĵ򿪱�����ʧ�ܣ���ʱ����Ϊ30�����ڵ���
		printf("VideoCapture inWidth:%d, inHeight:%d, fps:%d\n", inWidth, inHeight, fps);
		fps = 30;

		//2. ��ʼ����ʽת��������
		vsc = sws_getCachedContext(vsc,
			inWidth, inHeight, AV_PIX_FMT_BGR24,//ԭ��ߣ����ظ�ʽ
			inWidth, inHeight, AV_PIX_FMT_YUV420P,//Ŀ���ߣ����ظ�ʽ
			SWS_BICUBIC,//�ߴ�仯ʹ���㷨
			0, 0, 0);
		if (!vsc) {
			throw exception("sws_getCachedContext failed!");
		}

		//��������ݽṹ
		yuv = av_frame_alloc();
		yuv->format = AV_PIX_FMT_YUV420P;
		yuv->width = inWidth;
		yuv->height = inHeight;
		yuv->pts = 0;
		//�����ڴ�
		int ret = av_frame_get_buffer(yuv, 32);
		if (ret != 0) {
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			throw exception(buf);
		}

		//4 ��ʼ������������
		//a �ҵ�������
		AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
		if (!codec) {
			throw exception("Can't find h264 encoder!");
		}
		//b ����������������
		vc = avcodec_alloc_context3(codec);
		if (!vc) {
			throw exception("avcodec_alloc_context3 failed!");
		}
		//c ���ñ���������
		vc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;//ȫ�ֲ���
		vc->codec_id = codec->id;
		vc->thread_count = 8;
		vc->bit_rate = 50 * 1024 * 8;//ѹ����ÿ����Ƶ��bitλ��С 50KB
		vc->width = inWidth;
		vc->height = inHeight;
		vc->time_base = { 1, fps };
		vc->framerate = { fps, 1 };
		//������Ĵ�С������֡һ���ؼ�֡
		vc->gop_size = 60;
		vc->max_b_frames = 0;
		vc->pix_fmt = AV_PIX_FMT_YUV420P;
		//d �򿪱�����������
		ret = avcodec_open2(vc, 0, 0);
		if (ret != 0) {
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			throw exception(buf);
		}
		cout << "avcodec_open2 success!" << endl;

		//5 �����װ������Ƶ������
		//a ���������װ��������
		ret = avformat_alloc_output_context2(&oc, 0, "flv", outurl);
		if (ret != 0) {
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			throw exception(buf);
		}
		//b �����Ƶ��
		AVStream* vs = avformat_new_stream(oc, NULL);
		if (!vs) {
			throw exception("avformat_new_stream failed!");
		}
		vs->codecpar->codec_tag = 0;
		//�ӱ��������Ʋ���
		avcodec_parameters_from_context(vs->codecpar, vc);

		av_dump_format(oc, 0, outurl, 1);

		//��rtmp���������IO
		ret = avio_open(&oc->pb, outurl, AVIO_FLAG_WRITE);
		if (ret != 0) {
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			throw exception(buf);
		}
		//д���װͷ
		ret = avformat_write_header(oc, NULL);
		if (ret != 0) {
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			throw exception(buf);
		}

		AVPacket packet;
		av_init_packet(&packet);
		int vpts = 0;
		for (;;) {
			//��ȡrtsp��Ƶ֡��������Ƶ֡
			if (!cam.grab()) {
				continue;
			}
			//yuvת��Ϊrgb
			if (!cam.retrieve(frame)) {
				continue;
			}
			imshow("video", frame);
			waitKey(1);

			//rgb to yuv
			//��������ݽṹ
			uint8_t* indata[AV_NUM_DATA_POINTERS] = { 0 };
			//indata[0] bgrbgrbgr
			//plane indata[0]bbbbb indata[1]ggggg indata[2]rrrrr
			indata[0] = frame.data;
			int insize[AV_NUM_DATA_POINTERS] = { 0 };
			//һ��(��)���ݵ��ֽ���
			insize[0] = frame.cols * frame.elemSize();

			int h = sws_scale(vsc, indata, insize, 0, frame.rows, //ԭ����
				yuv->data, yuv->linesize);
			if (h <= 0) {
				continue;
			}
			//cout << h << " " << flush;

			//h264����
			yuv->pts = vpts;
			vpts++;
			ret = avcodec_send_frame(vc, yuv);
			if (ret != 0) {
				continue;
			}
			ret = avcodec_receive_packet(vc, &packet);//���ձ�������������
			if ((ret != 0) || (packet.size > 0)) {
				//cout << "*" << packet.size << flush;
			} else {
				continue;
			}

			//����
			//ʱ���׼��ת��������������ʱ���׼ת��Ϊ���Ļ�׼���ٽ�������
			packet.pts = av_rescale_q(packet.pts, vc->time_base, vs->time_base);
			packet.dts = av_rescale_q(packet.dts, vc->time_base, vs->time_base);
			packet.duration = av_rescale_q(packet.duration, vc->time_base, vs->time_base);
			packet.pos = -1;
			ret = av_interleaved_write_frame(oc, &packet);
			if (ret == 0) {
				//cout << "#" << flush;
			}
		}
	}
	catch (exception& e) {
		if (cam.isOpened())
			cam.release();
		if (vsc) {
			sws_freeContext(vsc);
			vsc = NULL;
		}
		if (vc) {
			avcodec_close(vc);
			avcodec_free_context(&vc);
		}
		if (oc) {
			avio_closep(&oc->pb);
			avformat_free_context(oc);
		}
		cerr << e.what() << endl;
	}
	getchar();
	return 0;
}