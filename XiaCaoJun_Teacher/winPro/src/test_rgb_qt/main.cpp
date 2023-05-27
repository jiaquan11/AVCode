#include "testrgb.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    TestRGB w;
    w.show();
    return a.exec();
}
