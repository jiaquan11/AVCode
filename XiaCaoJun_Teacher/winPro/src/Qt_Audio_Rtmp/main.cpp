#include <QtCore/QCoreApplication>
#include <QAudioFormat>
#include <QAudioDeviceInfo>
#include <QAudioInput>
#include <QThread>

#include <iostream>
using namespace std;

extern "C" {
#include <libswresample/swresample.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#pragma comment(lib, "swresample.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avformat.lib")

int main(int argc, char* argv[]) {
	QCoreApplication a(argc, argv);

	//nginx-rtmp 直播服务器推流url
	char* outurl = "rtmp://192.168.141.128/myapp/";

	//注册所有的封装器
	av_register_all();
	//注册所有的编解码器
	avcodec_register_all();
	//注册所有网络协议
	avformat_network_init();

	//音频采集参数设置
	int sampleRate = 44100;
	int channels = 2;
	int sampleByte = 2;
	AVSampleFormat inSampleFmt = AV_SAMPLE_FMT_S16;
	/*音频编码器输入的数据类型，为FLOAT类型，就是音频编码器是针对Float类型的pcm数据进行编码，
	所以采集好的音频pcm数据一般是S16类型，需要进行重采样
	*/
	AVSampleFormat outSampleFmt = AV_SAMPLE_FMT_FLTP;

	//1.qt音频开始录制
	QAudioFormat fmt;
	fmt.setSampleRate(sampleRate);
	fmt.setChannelCount(channels);
	fmt.setSampleSize(sampleByte * 8);//16位
	fmt.setCodec("audio/pcm");
	fmt.setByteOrder(QAudioFormat::LittleEndian);//小端模式
	fmt.setSampleType(QAudioFormat::UnSignedInt);
	QAudioDeviceInfo info = QAudioDeviceInfo::defaultInputDevice();//获取音频设备
	if (!info.isFormatSupported(fmt)) {
		cout << "Audio format not support!" << endl;
		fmt = info.nearestFormat(fmt);
	}

	QAudioInput* input = new QAudioInput(fmt);
	//开始录制音频
	QIODevice* io = input->start();//开启Qt音频录制，一旦开启会一直在进行采集，并送入数据缓冲区中

	//2音频重采样，上下文初始化
	SwrContext* asc = NULL;
	asc = swr_alloc_set_opts(asc,
		av_get_default_channel_layout(channels), outSampleFmt, sampleRate,//输出格式
		av_get_default_channel_layout(channels), inSampleFmt, sampleRate, 0, 0);//输入格式
	if (!asc) {
		cout << "swr_alloc_set_opts failed!" << endl;
		getchar();
		return -1;
	}
	int ret = swr_init(asc);
	if (ret != 0) {
		char err[1024] = { 0 };
		av_strerror(ret, err, sizeof(err) - 1);
		cout << err << endl;
		getchar();
		return -1;
	}
	cout << "音频重采样上下文初始化成功！" << endl;

	//3 音频重采样输出空间分配
	AVFrame* pcm = av_frame_alloc();
	pcm->format = outSampleFmt;
	pcm->channels = channels;
	pcm->channel_layout = av_get_default_channel_layout(channels);
	pcm->nb_samples = 1024;//一帧音频一通道的采样数量
	ret = av_frame_get_buffer(pcm, 0);//给pcm分配存储空间
	if (ret != 0) {
		char err[1024] = { 0 };
		av_strerror(ret, err, sizeof(err) - 1);
		cout << err << endl;
		getchar();
		return -1;
	}

	//4 初始化音频编码器
	AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
	if (!codec) {
		cout << "avcodec_find_encoder AV_CODEC_ID_AAC failed!" << endl;
		getchar();
		return -1;
	}
	//音频编码器上下文
	AVCodecContext* ac = avcodec_alloc_context3(codec);
	if (!ac) {
		cout << "avcodec_alloc_context3 failed" << endl;
		getchar();
		return -1;
	}
	cout << "avcodec_alloc_context3 success!" << endl;

	ac->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	ac->thread_count = 8;
	ac->bit_rate = 40000;
	ac->sample_rate = sampleRate;
	ac->sample_fmt = outSampleFmt;
	ac->channels = channels;
	ac->channel_layout = av_get_default_channel_layout(channels);

	//打开音频编码器
	ret = avcodec_open2(ac, 0, 0);
	if (ret != 0) {
		char err[1024] = { 0 };
		av_strerror(ret, err, sizeof(err) - 1);
		cout << err << endl;
		getchar();
		return -1;
	}
	cout << "avcodec_open2 success!" << endl;

	//5 输出封装器和音频流配置
	//a 创建输出封装器上下文
	AVFormatContext* oc = NULL;
	ret = avformat_alloc_output_context2(&oc, 0, "flv", outurl);
	if (ret != 0) {
		char err[1024] = { 0 };
		av_strerror(ret, err, sizeof(err) - 1);
		cout << err << endl;
		getchar();
		return -1;
	}
	//b 添加音频流
	AVStream* as = avformat_new_stream(oc, NULL);
	if (!as) {
		throw exception("avformat_new_stream failed!");
	}
	as->codecpar->codec_tag = 0;
	//从编码器复制参数
	avcodec_parameters_from_context(as->codecpar, ac);

	av_dump_format(oc, 0, outurl, 1);

	//打开rtmp的网络输出IO
	ret = avio_open(&oc->pb, outurl, AVIO_FLAG_WRITE);
	if (ret != 0) {
		char err[1024] = { 0 };
		av_strerror(ret, err, sizeof(err) - 1);
		cout << err << endl;
		getchar();
		return -1;
	}
	//写入封装头
	ret = avformat_write_header(oc, NULL);
	if (ret != 0) {
		char err[1024] = { 0 };
		av_strerror(ret, err, sizeof(err) - 1);
		cout << err << endl;
		getchar();
		return -1;
	}

	//一次读取一帧音频的字节数
	int readSize = pcm->nb_samples * channels * sampleByte;
	char* buf = new char[readSize];
	int apts = 0;

	AVPacket pkt = { 0 };
	for (;;) {
		//一次读取一帧音频
		if (input->bytesReady() < readSize) {//查看Qt录制器缓冲中的数据
			QThread::msleep(1);
			continue;
		}
		int size = 0;
		while (size != readSize) {//读取固定字节数的pcm数据，一般是4096个字节，读满固定buf为止
			int len = io->read(buf + size, readSize - size);//从缓冲区中读取pcm数据
			if (len < 0) break;//读取缓冲中的数据异常，直接退出
			size += len;
		}

		if (size != readSize) continue;

		//已经读一帧源数据

		//重采样源数据
		const uint8_t* indata[AV_NUM_DATA_POINTERS] = { 0 };
		indata[0] = (uint8_t*)buf;
		int len = swr_convert(asc, pcm->data, pcm->nb_samples,//输出参数，输出存储地址
			indata, pcm->nb_samples);
		//cout << len << " ";

		//pts运算
		//nb_sample/sample_rate = 一帧音频的秒数sec
		//timebase pts = sec*timebase.den
		pcm->pts = apts;//音频编码器编码前对AVFrame需要设置一个时间戳
		apts += av_rescale_q(pcm->nb_samples, { 1, sampleRate }, ac->time_base);
		cout << "apts: " << apts << " " << " ac.time_base.num" << ac->time_base.num << " ac.time_base.den: " << ac->time_base.den << flush;
		int ret = avcodec_send_frame(ac, pcm);
		if (ret != 0) continue;

		av_packet_unref(&pkt);
		ret = avcodec_receive_packet(ac, &pkt);
		if (ret != 0) {
			continue;
		}
		//cout << pkt.size << " " << flush;

		//推流
		pkt.pts = av_rescale_q(pkt.pts, ac->time_base, as->time_base);
		pkt.dts = av_rescale_q(pkt.dts, ac->time_base, as->time_base);
		pkt.duration = av_rescale_q(pkt.duration, ac->time_base, as->time_base);
		pkt.pos = -1;
		ret = av_interleaved_write_frame(oc, &pkt);
		if (ret == 0) {
			//cout << "#" << flush;
		}
	}

	delete[]buf;
	getchar();
	return a.exec();
}
