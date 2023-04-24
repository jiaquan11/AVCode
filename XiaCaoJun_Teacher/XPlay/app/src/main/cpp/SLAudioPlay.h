#ifndef _SLAUDIOPLAY_H_
#define _SLAUDIOPLAY_H_

#include "IAudioPlay.h"

class SLAudioPlay : public IAudioPlay {
public:
    SLAudioPlay();

    virtual ~SLAudioPlay();

public:
    virtual bool StartPlay(XParameter out);

    virtual void Close();

    void PlayCall(void *bufq);

protected:
    unsigned char *buf = 0;
    std::mutex mux;
};

#endif
