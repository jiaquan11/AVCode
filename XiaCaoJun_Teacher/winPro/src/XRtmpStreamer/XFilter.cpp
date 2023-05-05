#include "XFilter.h"
#include "XBilateralFilter.h"
#include <iostream>
using namespace std;

XFilter::XFilter() {

}

XFilter::~XFilter() {

}

XFilter* XFilter::Get(XFilterType t) {
	static XBilateralFilter xbf;

	switch (t) {
	case XBILATERAL://双边滤波
		return &xbf;

	default:
		break;
	}
}

bool XFilter::Set(std::string key, double value) {
	if (paras.find(key) == paras.end()) {//不符合要求输入键
		cout << "para " << key << " is not support!" << endl;
		return false;
	}
	paras[key] = value;
	return true;
}