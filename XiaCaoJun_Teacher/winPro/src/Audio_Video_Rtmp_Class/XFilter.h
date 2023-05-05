#pragma once
#include <string>
#include <map>

enum XFilterType{
	XBILATERAL	//双边滤波(美颜磨皮算法)
};

namespace cv {
	class Mat;
}

class XFilter {
protected:
	XFilter();

public:
	virtual ~XFilter();

public:
	static XFilter *Get(XFilterType t = XBILATERAL);//默认类型是双边磨皮滤镜

	virtual bool Set(std::string key, double value);

	virtual bool Filter(cv::Mat *src, cv::Mat *des) = 0;//纯虚函数，子类必须得实现

protected:
	std::map<std::string, double> paras;//映射表
};

