#include <string>
#include "log/android_log.h"
#include "opengl/opengl.h"

#define DELETE_LOCAL_REF(env, obj)  if(obj!=NULL){env->DeleteLocalRef(obj);obj=NULL;}
static const char *const kClassPathName = "com/jiaquan/opengl/NativeOpengl";

JavaVM *g_java_vm = NULL;
Opengl *g_opengl = NULL;
JNIEXPORT void JNICALL SurfaceCreate(JNIEnv *env, jobject thiz, jobject surface) {
    if (g_opengl == NULL) {
        g_opengl = new Opengl();
    }
    g_opengl->OnSurfaceCreate(g_java_vm, env, thiz, surface);
}

JNIEXPORT void JNICALL SurfaceChange(JNIEnv *env, jobject thiz, jint surface_width, jint surface_height) {
    if (g_opengl != NULL) {
        g_opengl->OnSurfaceChange(surface_width, surface_height);
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

JNIEXPORT void JNICALL SetImgData(JNIEnv *env, jobject thiz, jint image_width, jint image_height, jint size, jbyteArray data_array) {
    jbyte *data = env->GetByteArrayElements(data_array, NULL);
    if (g_opengl != NULL) {
        g_opengl->SetImgData(image_width, image_height, size, data);
    }
    env->ReleaseByteArrayElements(data_array, data, 0);
}

JNIEXPORT void JNICALL SetYuvData(JNIEnv *env, jobject thiz, jint yuv_width, jint yuv_height, jbyteArray y_array, jbyteArray u_array, jbyteArray v_array) {
    jbyte *y_data = env->GetByteArrayElements(y_array, NULL);
    jbyte *u_data = env->GetByteArrayElements(u_array, NULL);
    jbyte *v_data = env->GetByteArrayElements(v_array, NULL);
    if (g_opengl != NULL) {
        g_opengl->SetYuvData(yuv_width, yuv_height, y_data, u_data, v_data);
    }
    env->ReleaseByteArrayElements(y_array, y_data, 0);
    env->ReleaseByteArrayElements(u_array, u_data, 0);
    env->ReleaseByteArrayElements(v_array, v_data, 0);
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
    g_java_vm = vm;
    JNIEnv *env;
    if (g_java_vm->GetEnv((void **) (&env), JNI_VERSION_1_6) != JNI_OK) {
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

    if (g_java_vm != NULL) {
        g_java_vm = NULL;
    }
    LOGI("JNI_OnUnload out");
}