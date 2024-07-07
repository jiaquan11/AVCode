#ifndef MYPLAYER_WLPCMBEAN_H_
#define MYPLAYER_WLPCMBEAN_H_

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * PCM数据Bean
 */
class WLPcmBean {
public:
    WLPcmBean(char *buffer, int size);

    ~WLPcmBean();

public:
    char *m_buffer;
    int m_buffsize;
};

#endif //MYPLAYER_WLPCMBEAN_H_
