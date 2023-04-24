#ifndef _FFRESAMPLE_H_
#define _FFRESAMPLE_H_

#include "IResample.h"

struct SwrContext;

class FFResample : public IResample {
public:
    virtual bool Open(XParameter in, XParameter out = XParameter());

    virtual void Close();

    virtual XData Resample(XData indata);

protected:
    SwrContext *actx = 0;
    std::mutex mux;
};

#endif
