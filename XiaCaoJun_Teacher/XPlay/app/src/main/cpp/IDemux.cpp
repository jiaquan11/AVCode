#include "IDemux.h"

//demxuer的线程处理函数
void IDemux::Main() {
    while (!isExit) {
        if (IsPause()) {//播放暂停状态，持续休眠
            XSleep(2);
            continue;
        }
        XData d = Read();
        if (d.size > 0) {
            Notify(d);//通知观察者处理
        } else {//获取不到数据休眠2毫秒
            XSleep(2);
        }
    }
}