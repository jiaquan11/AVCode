#ifndef _XPARAMETER_H_
#define _XPARAMETER_H_

struct AVCodecParameters;

class XParameter {
public:
    AVCodecParameters *para = 0;
    int channels = 2;
    int sample_rate = 44100;
};

#endif
