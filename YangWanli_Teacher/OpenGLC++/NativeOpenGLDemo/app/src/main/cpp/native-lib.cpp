#include <string>
#include "log/android_log.h"
#include "opengl/opengl.h"

#define DELETE_LOCAL_REF(env, obj)  if(obj!=NULL){env->DeleteLocalRef(obj);obj=NULL;}
static const char *const kClassPathName = "com/jiaquan/opengl/NativeOpengl";

Opengl *g_opengl = NULL;
JNIEXPORT void JNICALL SurfaceCreate(JNIEnv *env, jobject thiz, jobject surface) {
    if (g_opengl == NULL) {
        g_opengl = new Opengl();
    }
    g_opengl->OnSurfaceCreate(env, surface);
}

JNIEXPORT void JNICALL SurfaceChange(JNIEnv *env, jobject thiz, jint width, jint height) {
    if (g_opengl != NULL) {
        g_opengl->OnSurfaceChange(width, height);
    }
}

JNIEXPORT void JNICALL SurfaceDestroy(JNIEnv *env, jobject thiz) {
    if (g_opengl != NULL) {
        g_opengl->OnSurfaceDestroy();
        delete g_opengl;
        g_opengl = NULL;
    }
}

JNIEXPORT void JNICALL SurfaceChangeFilter(JNIEnv *env, jobject thiz) {
    if (g_opengl != NULL) {
        g_opengl->OnSurfaceChangeFilter();
    }
}

JNIEXPORT void JNICALL SetImgData(JNIEnv *env, jobject thiz, jint image_width, jint image_height, jint size, jbyteArray data_array) {//传入解码后的图像rgba数据
    jbyte *data = env->GetByteArrayElements(data_array, NULL);
    if (g_opengl != NULL) {
        g_opengl->SetImgData(image_width, image_height, size, data);
    }
    env->ReleaseByteArrayElements(data_array, data, 0);
}

JNIEXPORT void JNICALL SetYuvData(JNIEnv *env, jobject thiz, jint w, jint h, jbyteArray y_array, jbyteArray u_array, jbyteArray v_array) {
    jbyte *ydata = env->GetByteArrayElements(y_array, NULL);
    jbyte *udata = env->GetByteArrayElements(u_array, NULL);
    jbyte *vdata = env->GetByteArrayElements(v_array, NULL);
    if (g_opengl != NULL) {
        g_opengl->SetYuvData(ydata, udata, vdata, w, h);
    }
    env->ReleaseByteArrayElements(y_array, ydata, 0);
    env->ReleaseByteArrayElements(u_array, udata, 0);
    env->ReleaseByteArrayElements(v_array, vdata, 0);
}

static JNINativeMethod gMethods[] = {
        {"nativeSurfaceCreate",         "(Landroid/view/Surface;)V",  (void *) SurfaceCreate},
        {"nativeSurfaceChange",         "(II)V",                      (void *) SurfaceChange},
        {"nativeSurfaceDestroy",        "()V",                        (void *) SurfaceDestroy},
        {"nativeSurfaceChangeFilter",   "()V",                        (void *) SurfaceChangeFilter},
        {"nativeSetImgData",            "(III[B)V",                   (void *) SetImgData},
        {"nativeSetYuvData",            "(II[B[B[B)V",                (void *) SetYuvData},
};

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    if (vm->GetEnv((void **) (&env), JNI_VERSION_1_6) != JNI_OK) {
        LOGE("JNI_OnLoad GetEnv failed!");
        return -1;
    }
    assert(env != NULL);

    jclass clazz = env->FindClass(kClassPathName);
    if (clazz == NULL) {
        LOGE("class not found. %s", kClassPathName);
        return -1;
    }

    if (env->RegisterNatives(clazz, gMethods, sizeof(gMethods) / sizeof(gMethods[0])) < 0) {
        LOGE("RegisterNatives failed!");
        return -1;
    }
    DELETE_LOCAL_REF(env, clazz);
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *vm, void *reserved) {
    LOGI("JNI_OnUnload in");
    JNIEnv *env;
    if (vm->GetEnv((void **) (&env), JNI_VERSION_1_6) != JNI_OK) {
        LOGE("JNI_OnUnload GetEnv failed!");
        return;
    }
    assert(env != NULL);
    LOGI("JNI_OnUnload out");
}