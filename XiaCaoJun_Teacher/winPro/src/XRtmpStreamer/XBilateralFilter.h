#pragma once
#include "XFilter.h"
class XBilateralFilter : public XFilter {
public:
	XBilateralFilter();

	virtual ~XBilateralFilter();

public:
	bool Filter(cv::Mat *src, cv::Mat *des);
};

