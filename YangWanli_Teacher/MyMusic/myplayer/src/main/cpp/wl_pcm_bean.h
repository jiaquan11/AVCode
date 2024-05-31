#ifndef MYPLAYER_WLPCMBEAN_H_
#define MYPLAYER_WLPCMBEAN_H_

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "soundtouch/include/SoundTouch.h"

/**
 * PCM数据Bean
 */
using namespace soundtouch;
class WLPcmBean {
public:
    WLPcmBean(SAMPLETYPE *buffer, int size);

    ~WLPcmBean();

public:
    char *m_buffer;
    int m_buffsize;
};

#endif //MYPLAYER_WLPCMBEAN_H_
