#ifndef _IRESAMPLE_H_
#define _IRESAMPLE_H_

#include "IObserver.h"
#include "XParameter.h"
#include "XLog.h"

class IResample : public IObserver {
public:
    virtual bool Open(XParameter in, XParameter out = XParameter()) = 0;

    virtual XData Resample(XData indata) = 0;

    virtual void Close() = 0;

    virtual void Update(XData data);

    int outChannels = 2;
    int outFormat = 1;
};

#endif
