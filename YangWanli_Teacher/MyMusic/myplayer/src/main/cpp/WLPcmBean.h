#ifndef _WLPCMBEAN_H_
#define _WLPCMBEAN_H_

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "soundtouch/include/SoundTouch.h"

using namespace soundtouch;

class WLPcmBean {
public:
    WLPcmBean(SAMPLETYPE *buffer, int size);

    ~WLPcmBean();

public:
    char *buffer;
    int buffsize;
};

#endif
