#pragma once

#include <QtWidgets/QWidget>
#include "ui_xrtmpstreamer.h"

class XRtmpStreamer : public QWidget
{
    Q_OBJECT

public:
    XRtmpStreamer(QWidget *parent = nullptr);
    ~XRtmpStreamer();

public slots:
    void startPushStream();

private:
    Ui::XRtmpStreamerClass ui;
};
