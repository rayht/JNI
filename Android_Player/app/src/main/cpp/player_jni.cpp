/**
 * @Author:Leiht
 * @Date:2018/10/1
 *
 * A bridge between Java(FfmpegPalyer) with ffmpeg.cpp
 */
#include <jni.h>
#include <string>
#include "ffmpeg.h"
#include "macro.h"
#include <android/native_window_jni.h>

#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,"JNI",__VA_ARGS__);

Ffmpeg *ffmpeg = 0;
JavaCallHelper *javaCallHelper = 0;

JavaVM *javaVm = 0;

ANativeWindow *window = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int JNI_OnLoad(JavaVM *vm, void *r) {
    LOGE("JNI_OnLoad JNI_OnLoad");
    javaVm = vm;
    return JNI_VERSION_1_6;
}

void JNI_OnUnload(JavaVM *vm, void *r) {
    LOGE("JNI_OnUnloadJNI_OnUnloadJNI_OnUnload");
    //释放锁资源
    pthread_mutex_destroy(&mutex);
    javaVm->DestroyJavaVM();
}

/**
 * 绘制，把RGBA绘制到ANativeWindow
 * @param data
 * @param lineszie
 * @param w
 * @param h
 */
void render(uint8_t *data, int lineszie, int w, int h) {
    pthread_mutex_lock(&mutex);

    if (!window) {
        pthread_mutex_unlock(&mutex);
        return;
    }

    //设置NativeWindow属性
    ANativeWindow_setBuffersGeometry(window, w, h, WINDOW_FORMAT_RGBA_8888);

    ANativeWindow_Buffer window_buffer;
    if (ANativeWindow_lock(window, &window_buffer, 0)) {
        ANativeWindow_release(window);
        window = 0;
        pthread_mutex_unlock(&mutex);
        return;
    }

    //填充rgb数据给dst_data
    uint8_t *dst_data = static_cast<uint8_t *>(window_buffer.bits);
    // stride：一行多少个数据（RGBA） *4
    //window_buffer.stride 一行的数据大小
    int dst_linesize = window_buffer.stride * 4;
    //一行一行的拷贝
    for (int i = 0; i < window_buffer.height; ++i) {
        //memcpy(dst_data , data, dst_linesize);
        memcpy(dst_data + i * dst_linesize, data + i * lineszie, dst_linesize);
    }
    ANativeWindow_unlockAndPost(window);

    pthread_mutex_unlock(&mutex);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_ffmpeg_player_FfmpegPlayer_native_1prepare(JNIEnv *env, jobject instance,
                                                    jstring dataSource_) {
    const char *dataSource = env->GetStringUTFChars(dataSource_, 0);

    javaCallHelper = new JavaCallHelper(javaVm, env, instance);
    ffmpeg = new Ffmpeg(javaCallHelper, dataSource);
    ffmpeg->setRenderFrameCallback(render);
    ffmpeg->prepare();

    env->ReleaseStringUTFChars(dataSource_, dataSource);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_ffmpeg_player_FfmpegPlayer_native_1start(JNIEnv *env, jobject instance) {
    if (ffmpeg) {
        ffmpeg->start();
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_ffmpeg_player_FfmpegPlayer_native_1set_1surface(JNIEnv *env, jobject instance,
                                                         jobject surface) {
    pthread_mutex_lock(&mutex);

    if (window) {
        //释放原来的NativeWindow，在SurfaceView改变的时候重新设置，如横竖屏切换
        ANativeWindow_release(window);
    }
    window = ANativeWindow_fromSurface(env, surface);

    pthread_mutex_unlock(&mutex);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_ffmpeg_player_FfmpegPlayer_native_1release(JNIEnv *env, jobject instance) {
    //释放掉window
    pthread_mutex_lock(&mutex);
    if (window) {
        //把老的释放
        ANativeWindow_release(window);
        window = 0;
    }
    pthread_mutex_unlock(&mutex);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_ffmpeg_player_FfmpegPlayer_native_1stop(JNIEnv *env, jobject instance) {
    LOGE("player_jni stop ...");
    if (ffmpeg) {
        ffmpeg->stop();
    }
    if(javaCallHelper) {
        LOGE("JNI : javaCallHelper release")
        delete(javaCallHelper);
        javaCallHelper = 0;
    }
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_ffmpeg_player_FfmpegPlayer_native_1getDuration(JNIEnv *env, jobject instance) {
    if (ffmpeg) {
        return ffmpeg->getDuration();
    }
    return 0;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_ffmpeg_player_FfmpegPlayer_native_1seek(JNIEnv *env, jobject instance, jint progress) {
    if (ffmpeg){
        ffmpeg->seek(progress);
    }
}