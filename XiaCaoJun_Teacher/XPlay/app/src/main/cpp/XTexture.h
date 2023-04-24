#ifndef _XTEXTURE_H_
#define _XTEXTURE_H_

enum XTextureType {
    XTEXTURE_YUV420P = 0,//Y 4 U 1 V 1
    XTEXTURE_NV12 = 25,//Y4 uv1
    XTEXTURE_NV21 = 26//Y4 vu1
};

class XTexture {//同样是单例模式
public:
    static XTexture *Create();

    virtual bool Init(void *win, XTextureType type = XTEXTURE_YUV420P) = 0;

    virtual void Draw(unsigned char *data[], int width, int height) = 0;

    virtual void Drop() = 0;

    virtual ~XTexture() {}

protected:
    XTexture() {};
};

#endif
