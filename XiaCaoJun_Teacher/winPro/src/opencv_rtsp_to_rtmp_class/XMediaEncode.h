#pragma once
struct AVFrame;
struct AVPacket;
struct AVCodecContext;

//����Ƶ����ӿ���
class XMediaEncode {
protected:
	XMediaEncode();

public:
	virtual ~XMediaEncode();

public:
	//������������
	static XMediaEncode* Get(unsigned char index = 0);

	//���麯�����������ʵ�ַ���
	//��Ƶ��������ʼ��
	virtual bool InitVideoCodec() = 0;

	//��Ƶ����
	virtual AVPacket* EncodeVideo(AVFrame* frame) = 0;

	//��ʼ�����ظ�ʽת���������ĳ�ʼ��
	virtual bool InitScale() = 0;

	virtual AVFrame* RGBToYUV(char* rgb) = 0;

public:
	AVCodecContext* vc = 0;//������������

	//�������
	int inWidth = 1280;
	int inHeight = 720;
	int inPixSize = 3;

	//�������
	int outWidth = 1280;
	int outHeight = 720;
	int bitrate = 4000000;//ѹ����ÿ����Ƶ��bitλ��С
	int fps = 30;
};

