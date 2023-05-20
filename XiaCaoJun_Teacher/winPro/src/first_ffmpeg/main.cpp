#include <iostream>
using namespace std;

extern "C" {
	#include <libavcodec/avcodec.h>
}

//预处理指令导入库
#pragma comment(lib, "avcodec.lib")

int main() {
	cout << "first ffmpeg" << endl;
	cout << avcodec_configuration() << endl;
	int version = avcodec_version();
	cout << version << endl;
	return 0;
}