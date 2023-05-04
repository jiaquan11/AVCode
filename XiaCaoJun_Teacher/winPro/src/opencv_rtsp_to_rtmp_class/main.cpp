#include <opencv2/highgui.hpp>
#include <iostream>
#include "XMediaEncode.h"
#include "XRtmp.h"

using namespace std;
using namespace cv;

int main(int argc, char* argv[]) {
	//���������rtsp url
	const char* inUrl = "rtsp://test:test123456@192.168.1.109";
	//nginx-rtmp ֱ������������url
	const char* outurl = "rtmp://192.168.141.128/myapp/";

	//�����������ظ�ʽת��
	XMediaEncode* me = XMediaEncode::Get(0);

	//��װ����������
	XRtmp* xr = XRtmp::Get(0);

	VideoCapture cam;
	Mat frame;
	namedWindow("video");

	int ret = 0;
	try
	{
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
		//3  ��ʼ����������ݽṹ
		me->inWidth = inWidth;
		me->inHeight = inHeight;
		me->outWidth = inWidth;
		me->outHeight = inHeight;
		me->InitScale();

		//4 ��ʼ������������
		//a �ҵ�������
		if (!me->InitVideoCodec()) {
			throw exception("InitVideoCodec failed!");
		}

		//5 �����װ������Ƶ������
		xr->Init(outurl);

		//�����Ƶ��
		xr->AddStream(me->vc);
		xr->SendHead();

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
			me->inPixSize = frame.elemSize();
			AVFrame* yuv = me->RGBToYUV((char*)frame.data);
			if (!yuv) {
				continue;
			}

			//h264����
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