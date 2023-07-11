#include "JavaListener.h"

JavaListener::JavaListener(JavaVM *vm, JNIEnv *env, jobject obj) {
    jvm = vm;
    jobj = obj;
    jniEnv = env;

    jclass clz = env->GetObjectClass(jobj);//获取当前Java对象的Java类
    if (!clz) {
        return;
    }
    jmethodId = env->GetMethodID(clz, "onError", "(ILjava/lang/String;)V");//获取得到Java类中的方法
}

JavaListener::~JavaListener() {
    //相关的资源销毁操作没有弄 jobj
}

void JavaListener::onError(int threadType, int code, const char *msg) {
    if (threadType == 0) {//子线程
        JNIEnv *env;
        jvm->AttachCurrentThread(&env, 0);//子线程中需要重新加载到Java虚拟环境中
        jstring jmsg = env->NewStringUTF(msg);
        env->CallVoidMethod(jobj, jmethodId, code, jmsg);
        env->DeleteLocalRef(jmsg);

        jvm->DetachCurrentThread();
    } else if (threadType == 1) {//主线程
        jstring jmsg = jniEnv->NewStringUTF(msg);
        jniEnv->CallVoidMethod(jobj, jmethodId, code, jmsg);
        jniEnv->DeleteLocalRef(jmsg);
    }
}
