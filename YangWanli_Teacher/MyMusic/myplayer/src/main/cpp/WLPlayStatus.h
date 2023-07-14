#ifndef _WLPLAYSTATUS_H_
#define _WLPLAYSTATUS_H_

class WLPlayStatus {
public:
    WLPlayStatus();

    ~WLPlayStatus();

public:
    bool isExit = false;
    bool load = true;
    bool seek = false;
    bool pause = false;
};
#endif
