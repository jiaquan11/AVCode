#include <jni.h>
#include <string>
#include "log/android_log.h"
#include "pthread.h"

#define DELETE_LOCAL_REF(env, obj)  if(obj!=NULL){env->DeleteLocalRef(obj);obj=NULL;}

static const char *const kClassPathName = "com/jiaquan/jnithread/ThreadDemo";

pthread_t thread;

void *normalCallBack(void *data) {
    LOGI("create normal thread from C++");
    pthread_exit(&thread);
}

JNIEXPORT void JNICALL NormalThread(JNIEnv *env, jobject thiz) {
    pthread_create(&thread, NULL, normalCallBack, NULL);
}

#include "queue"
#include "unistd.h"
#include "java_listener.h"

pthread_t g_product;
pthread_t g_customer;
pthread_mutex_t g_mutex;
pthread_cond_t g_cond;
std::queue<int> g_queue;

//生产者线程 函数
void *productCallBack(void *data) {
    while (true) {
        pthread_mutex_lock(&g_mutex);
        g_queue.push(1);
        pthread_cond_signal(&g_cond);
        LOGI("生产者生产一个产品，通知消费者消费，产品数量为%d", g_queue.size());
        pthread_mutex_unlock(&g_mutex);

        sleep(5);
    }
    pthread_exit(&g_product);
}

// 消费者线程函数
void *customerCallBack(void *data) {
    while (true) {
        pthread_mutex_lock(&g_mutex);
        if (g_queue.size() > 0) {
            g_queue.pop();
            LOGI("消费者消费一个产品，产品数量还剩余%d", g_queue.size());
        } else {
            LOGI("没有产品可以消费， 等待中....");
            pthread_cond_wait(&g_cond, &g_mutex);// 等待过程中会自动解锁，然后其它线程可以调度使用
        }
        pthread_mutex_unlock(&g_mutex);
        usleep(1000 * 500);
    }
    pthread_exit(&g_customer);
}

// 创建生产者和消费者两个子线程
JNIEXPORT void JNICALL MutexThread(JNIEnv *env, jobject thiz) {
    for (int i = 0; i < 10; ++i) {
        g_queue.push(i);//队列中插入10个数：0-9
    }

    pthread_mutex_init(&g_mutex, NULL);
    pthread_cond_init(&g_cond, NULL);

    pthread_create(&g_product, NULL, productCallBack, NULL);//创建生产者子线程
    pthread_create(&g_customer, NULL, customerCallBack, NULL);//创建消费者子线程
}

JavaVM *g_JavaVm;
java_listener *g_JavaListener;
pthread_t g_ChildThread;

void *childCallback(void *data) {
    java_listener *listener = (java_listener *) (data);
    listener->OnError(0, 101, "C++ call java method from child thread");
    delete listener;
    pthread_exit(&g_ChildThread);
}

JNIEXPORT void JNICALL CallBackFromC(JNIEnv *env, jobject thiz) {
    g_JavaListener = new java_listener(g_JavaVm, env, env->NewGlobalRef(thiz));// NewGlobalRef 全局引用
//    javaListener->onError(1, 100, "C++ call java method from main thread"); // 主线程中回调Java方法

    pthread_create(&g_ChildThread, NULL, childCallback, g_JavaListener);// 子线程中回调Java方法
}

static JNINativeMethod gMethods[] = {
        {"nativeNormalThread",  "()V",                  (void *)NormalThread},
        {"nativeMutexThread",   "()V",                  (void *)MutexThread},
        {"nativeCallBackFromC", "()V",                  (void *)CallBackFromC},
};

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    g_JavaVm = vm;
    if (g_JavaVm->GetEnv((void **) (&env), JNI_VERSION_1_6) != JNI_OK) {
        LOGE("JNI_OnLoad GetEnv failed!");
        return -1;
    }
    assert(env != NULL);

    jclass clazz = env->FindClass(kClassPathName);
    if (clazz == NULL) {
        LOGE("ThreadDemo class not found. %s", kClassPathName);
        return -1;
    }

    if (env->RegisterNatives(clazz, gMethods, sizeof(gMethods) / sizeof(gMethods[0])) < 0) {
        LOGE("RegisterNatives failed!");
        return -1;
    }

    DELETE_LOCAL_REF(env, clazz);
    return JNI_VERSION_1_6;
}