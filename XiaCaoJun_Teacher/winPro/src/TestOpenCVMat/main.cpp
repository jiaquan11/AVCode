#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>

using namespace cv;
using namespace std;

void PrintMs(const char* text = "") {
	static long long last = 0;
	long long cur = getTickCount();
	if (last == 0) {
		last = cur;
		return;
	}
	long long ms = 0;
	ms = ((double)(cur - last) / getTickFrequency()) * 1000;
	if (*text != 0) {
		printf("%s = %ldms\n", text, ms);
	}
	last = getTickCount();
}
int main(int argc, char* argv) {
	Mat mat(3000, 4000, CV_8UC3);
	//mat.create(3000, 4000, CV_8UC3);
	//元素大小字节数
	int es = mat.elemSize();
	int size = mat.rows * mat.cols * es;

	PrintMs();
	for (int i = 0; i < size; i += es) {
		mat.data[i] = 255;//B
		mat.data[i + 1] = 0;//G
		mat.data[i + 2] = 0;//R
	}
	PrintMs("mat.data ms");

	PrintMs();
	//地址遍历不一定连续的Mat  mat.step:表示一行的像素点数量
	for (int row = 0; row < mat.rows; row++) {
		for (int col = 0; col < mat.cols; col++) {
			(&mat.data[row * mat.step])[col * es] = 0;//B
			(&mat.data[row * mat.step])[col * es + 1] = 0;//G
			(&mat.data[row * mat.step])[col * es + 2] = 255;//R
		}
	}
	PrintMs("mat.step ms");

	PrintMs();
	//使用ptr遍历Mat
	for (int row = 0; row < mat.rows; row++) {
		for (int col = 0; col < mat.cols; col++) {
			Vec3b* c = mat.ptr<Vec3b>(row, col);
			c->val[0] = 0;//B
			c->val[1] = 255;//G
			c->val[2] = 0;//R
		}
	}
	PrintMs("mat.ptr");

	PrintMs();
	//使用at来遍历
	try
	{
		for (int row = 0; row < mat.rows; row++) {
			for (int col = 0; col < mat.cols; col++) {
				Vec3b& m = mat.at<Vec3b>(row, col);
				m[0] = 100;
				m[1] = 100;
				m[2] = 100;
			}
		}
	}
	catch (Exception& ex) {
		cout << ex.what() << endl;
	}
	PrintMs("mat.at");

	PrintMs();
	//使用迭代遍历
	auto it = mat.begin<Vec3b>();
	auto it_end = mat.end<Vec3b>();
	for (; it != it_end; it++)
	{
		(*it).val[0] = 0;//B
		(*it).val[1] = 0;//G
		(*it).val[2] = 255;//R
	}
	PrintMs("mat.itr");

	namedWindow("mat");
	imshow("mat", mat);
	waitKey(0);
	return 0;
}