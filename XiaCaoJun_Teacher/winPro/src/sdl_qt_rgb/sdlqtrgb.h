#pragma once

#include <QtWidgets/QWidget>
#include "ui_sdlqtrgb.h"

class SDLQtRGB : public QWidget
{
    Q_OBJECT

public:
    SDLQtRGB(QWidget *parent = nullptr);
    ~SDLQtRGB();

    void timerEvent(QTimerEvent* ev) override;

private:
    Ui::SDLQtRGBClass ui;
};
