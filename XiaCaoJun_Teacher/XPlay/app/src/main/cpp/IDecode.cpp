#include "IDecode.h"

//清除队列缓冲
void IDecode::Clear() {
    packsMutex.lock();
    while (!packs.empty()) {//清除packet缓冲队列
        packs.front().Drop();
        packs.pop_front();
    }
    pts = 0;
    synPts = 0;
    packsMutex.unlock();
}

//解码线程函数
void IDecode::Main() {//消费
    while (!isExit) {
        if (IsPause()) {//暂停状态，进入休眠
            XSleep(2);
            continue;
        }

        packsMutex.lock();
        //判断音视频同步
        if (!isAudio && (synPts > 0)) {//视频解码器与音频做同步
            if (synPts < pts) {//synPts为获取到的音频解码时间戳，音频慢于视频，视频解码流程进入休眠
                packsMutex.unlock();
                XSleep(1);
                continue;
            }
        }

        if (packs.empty()) {//缓冲队列为空，进入休眠，停止消费
            packsMutex.unlock();
            XSleep(1);
            continue;
        }
        //取出packet 消费者
        XData pack = packs.front();
        packs.pop_front();
        //发送数据到解码线程 一个数据包，可以解码多个结果
        if (this->SendPacket(pack)) {
            while (!isExit) {
                //获取解码数据
                XData frame = RecvFrame();
                if (!frame.data) break;
                //XLOGI("RecvFrame %d", frame.size);
                pts = frame.pts;
                //发送数据给观察者
                this->Notify(frame);
            }
        }
        pack.Drop();
        packsMutex.unlock();
    }
}

//由主体notify的数据
void IDecode::Update(XData pkt) {
    if (pkt.isAudio != isAudio) {//packet的类型与AVCodecContext中的type的类型不一致，则退出
        return;
    }

    while (!isExit) {
        packsMutex.lock();
        //阻塞
        if (packs.size() < maxList) {
            //生产者
            packs.push_back(pkt);//加入缓冲区
            packsMutex.unlock();
            break;
        }
        packsMutex.unlock();
        XSleep(1);//若上面缓冲区是满的，则会到这里进行循环延迟1秒
    }
}