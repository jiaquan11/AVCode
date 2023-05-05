#include "XData.h"
#include <string.h>
extern "C" {
#include <libavutil/time.h>
}

long long GetCurTime() {
	return av_gettime();
}

XData::XData(){

}

XData::XData(char* data, int size, long long p) {
	this->data = new char[size];
	memcpy(this->data, data, size);
	this->size = size;
	this->pts = p;
}

XData::~XData(){

}

void XData::Drop() {
	if (data) {
		delete data;
	}
	data = 0;
	size = 0;
	pts = 0;
}