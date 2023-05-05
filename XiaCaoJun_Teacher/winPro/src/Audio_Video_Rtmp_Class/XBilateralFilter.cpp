#include "XBilateralFilter.h"
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
using namespace cv;
using namespace std;

//Ë«±ßÄ¥Æ¤ÂË¾µ
XBilateralFilter::XBilateralFilter(){
	paras.insert(make_pair("d", 5));
}

XBilateralFilter::~XBilateralFilter(){

}

bool XBilateralFilter::Filter(cv::Mat *src, cv::Mat *des) {
	double d = paras["d"];//Í¨¹ı¼ü»ñµÃÖµ
	bilateralFilter(*src, *des, d, d * 2, d / 2);//opencvË«±ßÄ¥Æ¤ÂË¾µËã·¨
	return true;
}
