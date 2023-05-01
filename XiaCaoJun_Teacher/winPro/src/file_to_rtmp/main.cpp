#include <iostream>
using namespace std;

extern "C" {
#include "libavformat/avformat.h"
#include <libavutil/time.h>
}

/**
VS项目新项目需要配置的步骤：
1.设置常规-输出目录，如：..\..\bin\win32
2.调试-工作目录，如:..\..\bin\win32
3.C/C++ - 附加包含目录：..\..\include
4.链接-附加库目录: ..\..\lib\win32
5.添加库，下面两种方式多可以
*/
/*添加库有两种方式，一种是项目属性中的链接-输入-附加依赖项中添加相对应的库
第二种就是通过如下的预处理命令添加库
*/
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avcodec.lib")

int XError(int errNum) {
	char buf[1024] = { 0 };
	av_strerror(errNum, buf, sizeof(buf));
	cout << "error: " << buf << endl;
	getchar();
	return -1;
}

static double r2d(AVRational r) {
	return r.num == 0 || r.den == 0 ? 0. : (double)r.num / (double)r.den;
}

int main(int argc, char* argv[]) {
	cout << "file to rtmp test" << endl;
	char* inUrl = "demo_video.flv";
	char* outUrl = "rtmp://192.168.175.130/live/";//指定推流路径

	//初始化所有封装和解封装，flv mp4 mov mp3
	av_register_all();
	//初始化网络库
	avformat_network_init();

	//////////////////////////////////////////////////////////
	////输入流
	//1..打开文件，解封装
	//输入封装上下文
	AVFormatContext* ictx = NULL;
	//打开文件，解封文件头
	int ret = avformat_open_input(&ictx, inUrl, 0, 0);
	if (ret != 0) {
		cout << "avformat_open_input file " << inUrl << " failed!" << endl;
		return XError(ret);
	}
	cout << "open file " << inUrl << " Success!" << endl;

	//获取音视频流信息
	ret = avformat_find_stream_info(ictx, 0);
	if (ret != 0) {
		return XError(ret);
	}

	av_dump_format(ictx, 0, inUrl, 0);
	////////////////////////////////////////////////////////////
	/////输出流
	//创建输出流上下文
	AVFormatContext *octx = NULL;
	ret = avformat_alloc_output_context2(&octx, 0, "flv", outUrl);
	if (!octx) {
		return XError(ret);
	}
	cout << "octx create success!" << endl;

	//配置输出流
	//遍历输入的AVStream
	for (int i = 0; i < ictx->nb_streams; i++) {
		//创建输出流
		AVStream* out = avformat_new_stream(octx, NULL);
		if (!out) {
			return XError(0);
		}
		//复制配置信息,同于mp4
		//ret = avcodec_copy_context(out->codec, ictx->streams[i]->codec);
		ret = avcodec_parameters_copy(out->codecpar, ictx->streams[i]->codecpar);
		out->codec->codec_tag = 0;
	}

	av_dump_format(octx, 0, outUrl, 1);

	//rtmp推流
	//打开io
	ret = avio_open(&octx->pb, outUrl, AVIO_FLAG_WRITE);
	if (!octx->pb) {
		return XError(ret);
	}

	//写入头信息
	ret = avformat_write_header(octx, 0);
	if (ret < 0) {
		return XError(ret);
	}
	cout << "avformat_write_header " << ret << endl;

	//推流每一帧数据
	AVPacket pkt;
	long long startTime = av_gettime();
	for (;;) {
		ret = av_read_frame(ictx, &pkt);
		if (ret != 0) {
			break;
		}
		//计算转换pts dts
		AVRational itime = ictx->streams[pkt.stream_index]->time_base;
		AVRational otime = octx->streams[pkt.stream_index]->time_base;
		pkt.pts = av_rescale_q_rnd(pkt.pts, itime, otime, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.dts = av_rescale_q_rnd(pkt.dts, itime, otime, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.duration = av_rescale_q_rnd(pkt.duration, itime, otime, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.pos = -1;

		/*
		av_interleaved_write_frame内部会自动缓冲排序，及销毁pkt内存，
		无需外部再释放pkt
		*/
		/*视频帧推送速度控制
		控制策略：以读取的packet pts和当前系统时钟计数器进行计时对比
		如果推流的包的dts大于计时器的时间，就延时推送。保证按照当前计时器
		时钟进行推送
		*/
		if (ictx->streams[pkt.stream_index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			AVRational tb = ictx->streams[pkt.stream_index]->time_base;
			long long now = av_gettime() - startTime;
			long long dts = 0;
			long long pts = 0;
			dts = pkt.dts *(1000 * 1000 * r2d(tb));//微秒
			//pkt.pts = pkt.pts *(1000 * 1000 * r2d(tb));//微秒
			pts = pkt.pts *(1000 * 1000 * r2d(tb));//微秒
			cout << "pts: " << pts / 1000 << " dts: " << dts / 1000 << "| ";
			if (dts > now) {
				av_usleep(dts - now);
			}
		}
		ret = av_interleaved_write_frame(octx, &pkt);//推流
		if (ret < 0) {
			return XError(ret);
		}

		//新版本ffmpeg推荐使用av_packet_unref释放pkt内存，av_free_packet已被丢弃
		//av_packet_unref(&pkt);
	}

	getchar();
	return 0;
}