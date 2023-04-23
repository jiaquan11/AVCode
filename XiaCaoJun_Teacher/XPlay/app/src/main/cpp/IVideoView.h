#ifndef _IVIDEOVIEW_H_
#define _IVIDEOVIEW_H_

#include "XData.h"
#include "IObserver.h"

class IVideoView : public IObserver {
public:
    virtual void SetRender(void *win) = 0;

    virtual void Render(XData data) = 0;

    virtual void Update(XData data);

    virtual void Close() = 0;
};

#endif
