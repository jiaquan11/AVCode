#include <iostream>
extern "C"
{
#include <libavcodec/avcodec.h>
}
#pragma comment(lib,"avcodec.lib")
using namespace std;
int main(int argc, char* argv[])
{
	cout << "Test FFmpeg.club " << endl;
#ifdef WIN32
	cout << "32 λ����" << endl;
#else
	cout << "64 λ����" << endl;
#endif
	cout << avcodec_configuration() << endl;
	getchar();
	return 0;
}