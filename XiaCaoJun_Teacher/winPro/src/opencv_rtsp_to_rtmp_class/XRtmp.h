#pragma once
class AVCodecContext;
class AVPacket;

class XRtmp
{
protected:
	XRtmp();

public:
	virtual ~XRtmp();

public:
	//������������
	static XRtmp* Get(unsigned char index = 0);//����ģʽ

	//���麯�����������ʵ��
	//��ʼ����װ��������
	virtual bool Init(const char* url) = 0;

	//�����Ƶ����Ƶ��
	virtual bool AddStream(const AVCodecContext* c) = 0;

	//��rtmp����IO,���ͷ�װͷ
	virtual bool SendHead() = 0;

	//rtmp֡����
	virtual bool SendFrame(AVPacket* pkt) = 0;
};

