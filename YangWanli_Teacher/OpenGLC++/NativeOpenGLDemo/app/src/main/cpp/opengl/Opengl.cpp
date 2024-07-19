#include "opengl.h"

//函数指针的相关实现函数
void callback_SurfaceCreate(void *ctx) {
    Opengl *opengl = static_cast<Opengl *>(ctx);
    if (opengl != NULL) {
        if (opengl->baseOpengl != NULL) {
            opengl->baseOpengl->onCreate();
        }
    }
}

void callback_SurfaceChange(int width, int height, void *ctx) {
    Opengl *opengl = static_cast<Opengl *>(ctx);
    if (opengl != NULL) {
        if (opengl->baseOpengl != NULL) {
            opengl->baseOpengl->onChange(width, height);
        }
    }
}

void callback_SurfaceDraw(void *ctx) {
    Opengl *opengl = static_cast<Opengl *>(ctx);
    if (opengl != NULL) {
        if (opengl->baseOpengl != NULL) {
            opengl->baseOpengl->onDraw();
        }
    }
}

/*切换滤镜
 先把上次的滤镜资源类销毁，然后重新创建新的滤镜绘制类
 */
void callback_SurfaceChangeFilter(int width, int height, void *ctx) {
    Opengl *opengl = static_cast<Opengl *>(ctx);
    if (opengl != NULL) {
        if (opengl->baseOpengl != NULL) {
            opengl->baseOpengl->destroySource();
            opengl->baseOpengl->destroy();
            delete opengl->baseOpengl;
            opengl->baseOpengl = NULL;
        }

        //切换滤镜操作
        opengl->baseOpengl = new FilterTwo();
        opengl->baseOpengl->onCreate();
        opengl->baseOpengl->onChange(width, height);//屏幕显示宽高
        opengl->baseOpengl->setPixel(opengl->pixels, opengl->pic_width, opengl->pic_height);

        opengl->eglThread->notifyRender();
    }
}

void callback_SurfaceDestroy(void *ctx) {
    Opengl *opengl = static_cast<Opengl *>(ctx);
    if (opengl != NULL) {
        if (opengl->baseOpengl != NULL) {
            opengl->baseOpengl->destroy();
        }
    }
}

Opengl::Opengl() {

}

Opengl::~Opengl() {
    if (pixels != NULL) {
        free(pixels);
        pixels = NULL;
    }
}

void Opengl::OnSurfaceCreate(JNIEnv *env, jobject surface) {
    LOGI("Opengl OnSurfaceCreate in");
    nativeWindow = ANativeWindow_fromSurface(env, surface);

    eglThread = new EglThread();
    eglThread->setRenderType(OPENGL_RENDER_HANDLE);//设置渲染类型
    eglThread->callBackOnCreate(callback_SurfaceCreate, this);//设置函数指针
    eglThread->callBackOnChange(callback_SurfaceChange, this);
    eglThread->callBackOnDraw(callback_SurfaceDraw, this);
    eglThread->callBackOnChangeFilter(callback_SurfaceChangeFilter, this);
    eglThread->callBackOnDestroy(callback_SurfaceDestroy, this);

    baseOpengl = new FilterOne();//opengl绘制图片纹理

//    baseOpengl = new FilterYUV();//opengl绘制YUV视频

    eglThread->onSurfaceCreate(nativeWindow);//内部创建一个独立的子线程，用于EGL环境的操作
    LOGI("Opengl OnSurfaceCreate end");
}

void Opengl::OnSurfaceChange(int width, int height) {
    LOGI("Opengl OnSurfaceChange in width:%d, height:%d", width, height);
    if (eglThread != NULL) {
        if (baseOpengl != NULL) {
            baseOpengl->surface_width = width;
            baseOpengl->surface_height = height;
        }
        eglThread->onSurfaceChange(width, height);
    }
    LOGI("Opengl OnSurfaceChange end");
}

void Opengl::OnSurfaceDestroy() {
    LOGI("Opengl OnSurfaceDestroy in");
    if (eglThread != NULL) {
        eglThread->destroy();
        delete eglThread;
        eglThread = NULL;
    }
    if (baseOpengl != NULL) {
        baseOpengl->destroySource();
        delete baseOpengl;
        baseOpengl = NULL;
    }
    if (nativeWindow != NULL) {
        ANativeWindow_release(nativeWindow);
        nativeWindow = NULL;
    }
    if (pixels != NULL) {
        free(pixels);
        pixels = NULL;
    }
    LOGI("Opengl OnSurfaceDestroy end");
}

void Opengl::OnSurfaceChangeFilter() {
    if (eglThread != NULL) {
        eglThread->onSurfaceChangeFilter();
    }
}

void Opengl::SetImgData(int width, int height, int size, void* data) {
    pic_width = width;
    pic_height = height;
    if (pixels != NULL) {
        free(pixels);
        pixels = NULL;
    }
    pixels = malloc(size);
    memcpy(pixels, data, size);
    if (baseOpengl != NULL) {
        baseOpengl->setPixel(pixels, width, height);
    }

    if (eglThread != NULL) {
        eglThread->notifyRender();
    }
}

void Opengl::SetYuvData(void *y, void *u, void *v, int w, int h) {
    if (baseOpengl != NULL) {
        baseOpengl->setYuvData(y, u, v, w, h);
    }
    if (eglThread != NULL) {
        eglThread->notifyRender();
    }
}
