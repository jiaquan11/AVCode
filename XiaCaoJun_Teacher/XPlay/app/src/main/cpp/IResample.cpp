#include "IResample.h"

void IResample::Update(XData data) {
    XData d = this->Resample(data);
    //XLOGI("IResample Update %d", d.size);
    if (d.size > 0) {
        this->Notify(d);
    }
}