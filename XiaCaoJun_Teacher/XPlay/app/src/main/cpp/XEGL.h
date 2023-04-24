#ifndef _XEGL_H_
#define _XEGL_H_

class XEGL {//同样是单例模式
public:
    static XEGL *Get();

    virtual bool Init(void *win) = 0;

    virtual void Close() = 0;

    virtual void Draw() = 0;

protected:
    XEGL();
};

#endif
