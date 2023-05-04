#include <opencv2/highgui.hpp>
#include <iostream>
#include "XMediaEncode.h"
#include "XRtmp.h"

using namespace std;
using namespace cv;

int main(int argc, char* argv[]) {
	//海康相机的rtsp url
	const char* inUrl = "rtsp://test:test123456@192.168.1.109";
	//nginx-rtmp 直播服务器推流url
	const char* outurl = "rtmp://192.168.141.128/myapp/";

	//编码器和像素格式转换
	XMediaEncode* me = XMediaEncode::Get(0);

	//封装和推流对象
	XRtmp* xr = XRtmp::Get(0);

	VideoCapture cam;
	Mat frame;
	namedWindow("video");

	int ret = 0;
	try
	{
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
		//3  初始化输出的数据结构
		me->inWidth = inWidth;
		me->inHeight = inHeight;
		me->outWidth = inWidth;
		me->outHeight = inHeight;
		me->InitScale();

		//4 初始化编码上下文
		//a 找到编码器
		if (!me->InitVideoCodec()) {
			throw exception("InitVideoCodec failed!");
		}

		//5 输出封装器和视频流配置
		xr->Init(outurl);

		//添加视频流
		xr->AddStream(me->vc);
		xr->SendHead();

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
			me->inPixSize = frame.elemSize();
			AVFrame* yuv = me->RGBToYUV((char*)frame.data);
			if (!yuv) {
				continue;
			}

			//h264编码
			AVPacket* packet = me->EncodeVideo(yuv);
			if (!packet) continue;

			xr->SendFrame(packet);
		}
	} catch (exception& e) {
		if (cam.isOpened()) {
			cam.release();
		}
		cerr << e.what() << endl;
	}

	getchar();
	return 0;
}