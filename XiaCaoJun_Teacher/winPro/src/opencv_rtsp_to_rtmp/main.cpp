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
	//注册所有的封装器
	//av_register_all();
	//注册所有的编解码器
	//avcodec_register_all();
	//注册所有网络协议
	avformat_network_init();
	//海康相机的rtsp url
	const char* inUrl = "rtsp://test:test123456@192.168.1.109";
	//nginx-rtmp 直播服务器推流url
	const char* outurl = "rtmp://192.168.141.128/myapp/";

	VideoCapture cam;
	Mat frame;
	namedWindow("video");

	//像素格式转换上下文
	SwsContext* vsc = NULL;
	//输出的数据结构
	AVFrame* yuv = NULL;
	//编码器上下文
	AVCodecContext* vc = NULL;
	//rtmp flv封装器
	AVFormatContext* oc = NULL;

	try {
		/////////////////////////////////////////////
		///1 使用opencv打开rtsp相机
		//cam.open(inUrl);
		cam.open(0);
		if (!cam.isOpened()) {
			throw exception("cam open failed!");
		}
		cout << inUrl << "cam open success!" << endl;
		int inWidth = cam.get(CAP_PROP_FRAME_WIDTH);
		int inHeight = cam.get(CAP_PROP_FRAME_HEIGHT);
		int fps = cam.get(CAP_PROP_FPS);//这里获取的摄像头帧率可能为0，会导致后续的打开编码器失败，暂时设置为30，用于调试
		printf("VideoCapture inWidth:%d, inHeight:%d, fps:%d\n", inWidth, inHeight, fps);
		fps = 30;

		//2. 初始化格式转换上下文
		vsc = sws_getCachedContext(vsc,
			inWidth, inHeight, AV_PIX_FMT_BGR24,//原宽高，像素格式
			inWidth, inHeight, AV_PIX_FMT_YUV420P,//目标宽高，像素格式
			SWS_BICUBIC,//尺寸变化使用算法
			0, 0, 0);
		if (!vsc) {
			throw exception("sws_getCachedContext failed!");
		}

		//输出的数据结构
		yuv = av_frame_alloc();
		yuv->format = AV_PIX_FMT_YUV420P;
		yuv->width = inWidth;
		yuv->height = inHeight;
		yuv->pts = 0;
		//分配内存
		int ret = av_frame_get_buffer(yuv, 32);
		if (ret != 0) {
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			throw exception(buf);
		}

		//4 初始化编码上下文
		//a 找到编码器
		AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
		if (!codec) {
			throw exception("Can't find h264 encoder!");
		}
		//b 创建编码器上下文
		vc = avcodec_alloc_context3(codec);
		if (!vc) {
			throw exception("avcodec_alloc_context3 failed!");
		}
		//c 配置编码器参数
		vc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;//全局参数
		vc->codec_id = codec->id;
		vc->thread_count = 8;
		vc->bit_rate = 50 * 1024 * 8;//压缩后每秒视频的bit位大小 50KB
		vc->width = inWidth;
		vc->height = inHeight;
		vc->time_base = { 1, fps };
		vc->framerate = { fps, 1 };
		//画面组的大小，多少帧一个关键帧
		vc->gop_size = 60;
		vc->max_b_frames = 0;
		vc->pix_fmt = AV_PIX_FMT_YUV420P;
		//d 打开编码器上下文
		ret = avcodec_open2(vc, 0, 0);
		if (ret != 0) {
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			throw exception(buf);
		}
		cout << "avcodec_open2 success!" << endl;

		//5 输出封装器和视频流配置
		//a 创建输出封装器上下文
		ret = avformat_alloc_output_context2(&oc, 0, "flv", outurl);
		if (ret != 0) {
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			throw exception(buf);
		}
		//b 添加视频流
		AVStream* vs = avformat_new_stream(oc, NULL);
		if (!vs) {
			throw exception("avformat_new_stream failed!");
		}
		vs->codecpar->codec_tag = 0;
		//从编码器复制参数
		avcodec_parameters_from_context(vs->codecpar, vc);

		av_dump_format(oc, 0, outurl, 1);

		//打开rtmp的网络输出IO
		ret = avio_open(&oc->pb, outurl, AVIO_FLAG_WRITE);
		if (ret != 0) {
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			throw exception(buf);
		}
		//写入封装头
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
			//读取rtsp视频帧，解码视频帧
			if (!cam.grab()) {
				continue;
			}
			//yuv转换为rgb
			if (!cam.retrieve(frame)) {
				continue;
			}
			imshow("video", frame);
			waitKey(1);

			//rgb to yuv
			//输入的数据结构
			uint8_t* indata[AV_NUM_DATA_POINTERS] = { 0 };
			//indata[0] bgrbgrbgr
			//plane indata[0]bbbbb indata[1]ggggg indata[2]rrrrr
			indata[0] = frame.data;
			int insize[AV_NUM_DATA_POINTERS] = { 0 };
			//一行(宽)数据的字节数
			insize[0] = frame.cols * frame.elemSize();

			int h = sws_scale(vsc, indata, insize, 0, frame.rows, //原数据
				yuv->data, yuv->linesize);
			if (h <= 0) {
				continue;
			}
			//cout << h << " " << flush;

			//h264编码
			yuv->pts = vpts;
			vpts++;
			ret = avcodec_send_frame(vc, yuv);
			if (ret != 0) {
				continue;
			}
			ret = avcodec_receive_packet(vc, &packet);//接收编码后的码流数据
			if ((ret != 0) || (packet.size > 0)) {
				//cout << "*" << packet.size << flush;
			} else {
				continue;
			}

			//推流
			//时间基准的转换，将编码器的时间基准转换为流的基准，再进行推流
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