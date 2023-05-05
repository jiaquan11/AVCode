#include "XVideoCapture.h"
#include <opencv2/highgui.hpp>
#include <iostream>
using namespace cv;
using namespace std;
#pragma comment(lib, "opencv_world470d.lib")

class CXVideoCapture : public XVideoCapture {
public:
	VideoCapture cam;

public:
	bool Init(int camIndex = 0) {//打开本地摄像头
		cam.open(camIndex);
		if (!cam.isOpened()) {
			cout << "cam open failed!" << endl;
			return false;
		}
		cout << camIndex << " cam open success!" << endl;
		width = cam.get(CAP_PROP_FRAME_WIDTH);
		height = cam.get(CAP_PROP_FRAME_HEIGHT);
		fps = cam.get(CAP_PROP_FPS);
		if (fps == 0) {
			fps = 30;
		}
		return true;
	}

	bool Init(const char *url) {//打开网络摄像头
		cam.open(url);
		if (!cam.isOpened()) {
			cout << "cam open failed!" << endl;
			return false;
		}
		cout << url << " cam open success!" << endl;
		width = cam.get(CAP_PROP_FRAME_WIDTH);//摄像头宽
		height = cam.get(CAP_PROP_FRAME_HEIGHT);//摄像头高
		fps = cam.get(CAP_PROP_FPS);////摄像头帧率
		if (fps == 0) {
			fps = 30;
		}
		return true;
	}

	void Stop() {
		XDataThread::Stop();//停止视频采集线程

		if (cam.isOpened()) {//关闭摄像头资源
			cam.release();
		}
	}

	//在子线程中进行视频图像的采集
	void run() {
		Mat frame;
		while (!isExit) {
			if (!cam.read(frame)) {//采集视频图像
				msleep(1);
				continue;
			}
			if (frame.empty()) {
				msleep(1);
				continue;
			}

			fmutex.lock();
			int num = filters.size();
			for (int i = 0; i < num; i++) {//执行多个滤镜的算法操作
				Mat des;
				filters[i]->Filter(&frame, &des);
				frame = des;
			}
			fmutex.unlock();

			//确保数据是连续的
			long long pts = GetCurTime();//获取当前系统时间戳，这是当前采集的视频图像帧的采集时间戳
			XData d((char*)frame.data, frame.cols * frame.rows * frame.elemSize(), pts);
			Push(d);//采集的视频图像数据进行处理后，插入缓冲队列中
		}
	}
};

XVideoCapture::XVideoCapture() {

}

XVideoCapture::~XVideoCapture() {

}

XVideoCapture* XVideoCapture::Get(unsigned char index) {
	static CXVideoCapture cx[255];
	return &cx[index];
}
