#include "IPlayerProxy.h"
#include "FFPlayerBuilder.h"

void IPlayerProxy::Init(void *vm) {
    mux.lock();
    if (vm) {
        FFPlayerBuilder::InitHard(vm);
    }

    if (!player) {
        player = FFPlayerBuilder::Get()->BuilderPlayer();//构建一个播放器实例
    }
    mux.unlock();
}

void IPlayerProxy::InitView(void *win) {
    mux.lock();
    if (player) {
        player->InitView(win);
    }
    mux.unlock();
}

bool IPlayerProxy::Open(const char *path) {
    bool ret = false;
    mux.lock();
    if (player) {
        player->isHardDecode = isHardDecode;
        ret = player->Open(path);
    }
    mux.unlock();
    return ret;
}

void IPlayerProxy::Close() {
    mux.lock();
    if (player) {
        player->Close();
    }
    mux.unlock();
}

bool IPlayerProxy::Start() {
    bool ret = false;
    mux.lock();
    if (player) {
        ret = player->Start();
    }
    mux.unlock();
    return ret;
}

bool IPlayerProxy::Seek(double pos) {
    bool ret = false;
    mux.lock();
    if (player) {
        ret = player->Seek(pos);
    }
    mux.unlock();
    return ret;
}

void IPlayerProxy::SetPause(bool isP) {
    mux.lock();
    if (player) {
        player->SetPause(isP);
    }
    mux.unlock();
}

bool IPlayerProxy::IsPause() {
    bool ret = false;
    mux.lock();
    if (player) {
        ret = player->IsPause();
    }
    mux.unlock();
    return ret;
}

//获取当前的播放进度 0.0-1.0
double IPlayerProxy::PlayPos() {
    double pos = 0.0;
    mux.lock();
    if (player) {
        pos = player->PlayPos();
    }
    mux.unlock();
    return pos;
}