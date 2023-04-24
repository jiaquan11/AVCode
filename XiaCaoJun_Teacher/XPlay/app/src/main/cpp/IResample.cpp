#include "IResample.h"

void IResample::Update(XData data) {
    XData d = this->Resample(data);//重采样
    //XLOGI("IResample Update %d", d.size);
    if (d.size > 0) {
        this->Notify(d);
    }
}