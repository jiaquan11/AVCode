#include <opencv2/highgui.hpp>
#include <string>
#include <iostream>
using namespace std;
using namespace cv;

int main() {
	VideoCapture cam;
	string url = "rtsp://test:test123456@192.168.1.64";

	//if (cam.open(url)) {
	if (cam.open(0)) {
		cout << "open cam success!" << endl;
	}
	else {
		cout << "open cam failed!" << endl;
		waitKey(0);
		return -1;
	}

	namedWindow("video");
	Mat frame;
	for (;;) {
		cam.read(frame);
		imshow("video", frame);
		waitKey(1);
	}

	getchar();

	return 0;
}