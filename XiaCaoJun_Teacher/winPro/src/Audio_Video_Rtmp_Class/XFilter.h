#pragma once
#include <string>
#include <map>

enum XFilterType{
	XBILATERAL	//˫���˲�(����ĥƤ�㷨)
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
	static XFilter *Get(XFilterType t = XBILATERAL);//Ĭ��������˫��ĥƤ�˾�

	virtual bool Set(std::string key, double value);

	virtual bool Filter(cv::Mat *src, cv::Mat *des) = 0;//���麯������������ʵ��

protected:
	std::map<std::string, double> paras;//ӳ���
};

