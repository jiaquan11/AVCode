#include "IVideoView.h"

//视频解码YUV图像会持续通知过来进行update, IVideoView为VDecoder的观察者
void IVideoView::Update(XData data) {
    this->Render(data);
}