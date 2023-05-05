#include "XBilateralFilter.h"
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
using namespace cv;
using namespace std;

//˫��ĥƤ�˾�
XBilateralFilter::XBilateralFilter(){
	paras.insert(make_pair("d", 5));
}

XBilateralFilter::~XBilateralFilter(){

}

bool XBilateralFilter::Filter(cv::Mat *src, cv::Mat *des) {
	double d = paras["d"];//ͨ�������ֵ
	bilateralFilter(*src, *des, d, d * 2, d / 2);//opencv˫��ĥƤ�˾��㷨
	return true;
}
