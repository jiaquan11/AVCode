#ifndef _GLVIDEOVIEW_H_
#define _GLVIDEOVIEW_H_

#include "XData.h"
#include "IVideoView.h"

class XTexture;

class GLVideoView : public IVideoView {
public:
    virtual void SetRender(void *win);

    virtual void Render(XData data);

    virtual void Close();

protected:
    void *view = 0;
    XTexture *txt = 0;
    std::mutex mux;
};

#endif
