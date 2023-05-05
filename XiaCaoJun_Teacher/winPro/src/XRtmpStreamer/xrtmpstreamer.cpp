#include <iostream>
#include "xrtmpstreamer.h"
#include "XController.h"

using namespace std;

static bool isStream = false;

XRtmpStreamer::XRtmpStreamer(QWidget *parent)
    : QWidget(parent) 
{
    ui.setupUi(this);
}

XRtmpStreamer::~XRtmpStreamer()
{}

void XRtmpStreamer::startPushStream() {
	if (isStream) {
		isStream = false;
		ui.startButton->setText(QString::fromLocal8Bit("��ʼ"));
		XController::Get()->Stop();
	}
	else {
		isStream = true;
		ui.startButton->setText(QString::fromLocal8Bit("ֹͣ"));
		QString url = ui.inUrl->text();
		bool ok = false;
		int camIndex = url.toInt(&ok);//���ı����е�����ת��Ϊint����
		if (!ok) {
			XController::Get()->inUrl = url.toStdString();
		} else {
			XController::Get()->camIndex = camIndex;
		}
		XController::Get()->outUrl = ui.outUrl->text().toStdString();
		XController::Get()->Set("b", (ui.faceLevel->currentIndex() + 1) * 3);
		XController::Get()->Start();
	}
}
