#include "XController.h"
#include "XVideoCapture.h"
#include "XAudioRecord.h"
#include "XMediaEncode.h"
#include "XRtmp.h"
#include <iostream>
using namespace std;

XController::XController(){

}

XController::~XController(){

}

//设定美颜参数
bool XController::Set(std::string key, double value) {
	XFilter::Get()->Set(key, value);
	return true;
}

bool XController::Start() {
	//1 设置磨皮过滤器
	XVideoCapture::Get()->AddFilter(XFilter::Get());
	cout << "1 设置磨皮过滤器" << endl;

	//2 打开相机
	if (camIndex >= 0) {
		if (!XVideoCapture::Get()->Init(camIndex)) {
			err = "2 打开系统相机失败";
			cout << err << endl;
			return false;
		}
	}else if (!inUrl.empty()) {
		if (!XVideoCapture::Get()->Init(inUrl.c_str())) {
			err = "2 打开";
			err += inUrl;
			err += "相机失败";
			cout << err << endl;
			return false;
		}
	}else {
		err = "2 请设置相机参数";
		cout << err << endl;
		return false;
	}
	cout << "2 相机打开成功!" << endl;

	//3 QT音频开始录制
	if (!XAudioRecord::Get()->Init()) {
		err = "3 录音设备打开失败";
		cout << err << endl;
		return false;
	}
	cout << "3 录音设备打开成功!" << endl;

	//11 启动音视频录制线程 (分别进入各自子线程进行采集图像和音频)
	XAudioRecord::Get()->Start();
	XVideoCapture::Get()->Start();

	//音视频编码类
	//4 初始化格式转换上下文
	//初始化输出的数据结构
	XMediaEncode::Get()->inWidth = XVideoCapture::Get()->width;
	XMediaEncode::Get()->inHeight = XVideoCapture::Get()->height;
	XMediaEncode::Get()->outWidth = XVideoCapture::Get()->width;
	XMediaEncode::Get()->outHeight = XVideoCapture::Get()->height;
	if (!XMediaEncode::Get()->InitScale()) {
		err = "4 视频像素格式转换打开失败";
		cout << err << endl;
		return false;
	}
	cout << "4 视频像素格式转换打开成功!" << endl;

	//5 音频重采样上下文初始化
	XMediaEncode::Get()->channels = XAudioRecord::Get()->channels;
	XMediaEncode::Get()->nbSamples = XAudioRecord::Get()->nbSamples;
	XMediaEncode::Get()->sampleRate = XAudioRecord::Get()->sampleRate;
	if (!XMediaEncode::Get()->InitResample()) {
		err = "5 音频重采样上下文初始化失败!";
		cout << err << endl;
		return false;
	}
	cout << "5 音频重采样上下文初始化成功!" << endl;

	//6 初始化音频编码器
	if (!XMediaEncode::Get()->InitAudioCodec()) {
		err = "6 初始化音频编码器失败!";
		cout << err << endl;
		return false;
	}

	//7 初始化视频编码器
	if (!XMediaEncode::Get()->InitVideoCodec()) {
		err = "7 初始化视频编码器失败!";
		cout << err << endl;
		return false;
	}
	cout << "7 视频编码器初始化成功!" << endl;

	//8 创建输出封装器上下文
	if (!XRtmp::Get()->Init(outUrl.c_str())) {
		err = "8 创建输出封装器上下文失败!";
		cout << err << endl;
		return false;
	}
	cout << "8 创建输出封装器上下文成功" << endl;

	//9 添加音视频流
	vindex = XRtmp::Get()->AddStream(XMediaEncode::Get()->vc);
	aindex = XRtmp::Get()->AddStream(XMediaEncode::Get()->ac);
	if ((vindex < 0) || (aindex < 0)) {
		err = "9 添加音视频流失败!";
		cout << err << endl;
		return false;
	}
	cout << "9 添加音视频流成功!" << endl;

	//10 发送封装头
	if (!XRtmp::Get()->SendHead()) {
		err = "10 发送封装头失败!";
		cout << err << endl;
		return false;
	}

	/*将提前开启线程的缓冲数据先全部清空，保证缓冲队列中没有数据，同时开始新的图像数据和音频数据采集输入，
	这是为了音视频同步
	*/
	XAudioRecord::Get()->Clear();
	XVideoCapture::Get()->Clear();

	XDataThread::Start();//启动主流程线程
	return true;
}

void XController::Stop() {
	XDataThread::Stop();//停止主流程线程

	XAudioRecord::Get()->Stop();//停止音频采集线程
	XVideoCapture::Get()->Stop();//停止视频图像采集线程
	XMediaEncode::Get()->Close();//关闭编码器资源
	XRtmp::Get()->Close();//关闭推流器资源

	camIndex = -1;
	inUrl = "";
}

//线程函数
void XController::run() {
	long long beginTime = GetCurTime();//基准计时开始点 (主流程获取的系统当前时间戳)
	/*
	视频录制推流音视频同步逻辑:
	1.同时开启音频采集线程和视频采集线程，并获取当前采集到数据的时间点
	2.设置一个基准计时开始点
	3.音视频各自将获取数据的时间点减去时间基准点的差值，作为推流的时间戳
	4.如此可以较精准地保证推流同步
	*/
	while (!isExit) {
		//分别从缓冲队列中取出音频和视频的采集的数据
		XData ad = XAudioRecord::Get()->Pop();
		XData vd = XVideoCapture::Get()->Pop();
		if ((ad.size <= 0) && (vd.size <= 0)) {
			msleep(1);
			continue;
		}

		//处理音频
		if (ad.size > 0) {
			ad.pts = ad.pts - beginTime;//音频采集时间戳减去基准时间戳(以系统时间戳作为基准) 得到差值
			//重采样源数据
			XData pcm = XMediaEncode::Get()->Resample(ad);
			ad.Drop();//释放当前音频pcm数据的内存

			XData pkt = XMediaEncode::Get()->EncodeAudio(pcm);
			if (pkt.size > 0) {
				//推流
				if (XRtmp::Get()->SendFrame(pkt, aindex)) {
					cout << "#" << flush;
				}
			}
		}

		//处理视频
		if (vd.size > 0) {
			vd.pts = vd.pts - beginTime;//视频采集时间戳减去基准时间戳(以系统时间戳作为基准) 得到差值
			//cout << "vd pts" <<vd.pts<< flush;
			XData yuv = XMediaEncode::Get()->RGBToYUV(vd);
			vd.Drop();//释放当前视频图像rgb数据的内存

			XData pkt = XMediaEncode::Get()->EncodeVideo(yuv);
			cout << " V:" << pkt.size << flush;
			if (pkt.size > 0) {
				//推流
				if (XRtmp::Get()->SendFrame(pkt, vindex)) {
					cout << "@" << flush;
				}
			}
		}
	}
}

