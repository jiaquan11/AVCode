#include "XThread.h"

using namespace std;

//sleep毫秒
void XSleep(int mis) {
    chrono::milliseconds du(mis);
    this_thread::sleep_for(du);
}

//启动线程
bool XThread::Start() {
    isExit = false;
    isPause = false;
    thread th(&XThread::ThreadMain, this);//创建子线程，线程入口函数
    th.detach();//子线程分离
    return true;
}

//通过控制isExit变量安全停止线程(不一定)
void XThread::Stop() {
    isExit = true;
    for (int i = 0; i < 200; ++i) {//延迟200ms，等待子线程结束
        if (!isRunning) {//线程已经停止成功
            XLOGI("Stop 停止线程成功!");
            return;
        }
        XSleep(1);
    }
    XLOGI("Stop 停止线程超时!");
}

//设置是否播放暂停
void XThread::SetPause(bool isP) {
    isPause = isP;
    //等待100毫秒
    for (int i = 0; i < 10; ++i) {
        if (isPausing == isP) {
            break;
        }
        XSleep(10);
    }
}

bool XThread::IsPause() {
     isPausing = isPause;
    return isPause;
}

void XThread::ThreadMain() {
    XLOGI("线程函数进入");
    isRunning = true;
    Main();
    isRunning = false;
    XLOGI("线程函数退出");
}