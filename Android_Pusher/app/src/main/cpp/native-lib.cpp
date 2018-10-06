/**
 * @Author Leiht
 * @Date 2018/10/6
 */

#include <jni.h>
#include <string>
#include "librtmp/rtmp.h"
#include "VideoChannel.h"
#include "AudioChannel.h"
#include "safe_queue.h"
#include "macro.h"

SafeQueue<RTMPPacket *> packets;

VideoChannel *videoChannel = 0;

AudioChannel *audioChannel = 0;

int isStart = 0;
pthread_t pid_start;

int readyPushing = 0;
uint32_t start_time;

/**
 * 释放RTMPPacket函数及回调函数
 * @param packet
 */
void releasePackket(RTMPPacket *&packet) {
    if (packet) {
        RTMPPacket_Free(packet);
        delete (packet);
        packet = 0;
    }
}


void videoCallBack(RTMPPacket *packet) {
    if (packet) {
        //设置时间戳
        packet->m_nTimeStamp = RTMP_GetTime() - start_time;
        packets.push(packet);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_android_pusher_live_LivePusher_native_1init(JNIEnv *env, jobject instance) {
    //准备一个Video编码器的工具类 ：进行编码
    videoChannel = new VideoChannel;
    videoChannel->setVideoCallback(videoCallBack);
    //准备一个队列,打包好的数据 放入队列，在线程中统一的取出数据再发送给服务器
    packets.setReleaseCallback(releasePackket);
}

void *start(void *args) {
    char *url = static_cast<char *>(args);
    RTMP *rtmp = 0;
    do {
        rtmp = RTMP_Alloc();
        if (!rtmp) {
            LOGE("alloc rtmp失败");
            break;
        }
        RTMP_Init(rtmp);
        int ret = RTMP_SetupURL(rtmp, url);
        if (!ret) {
            LOGE("设置地址失败:%s", url);
            break;
        }
        //5s超时时间
        rtmp->Link.timeout = 5;
        RTMP_EnableWrite(rtmp);
        ret = RTMP_Connect(rtmp, 0);
        if (!ret) {
            LOGE("连接服务器:%s", url);
            break;
        }
        ret = RTMP_ConnectStream(rtmp, 0);
        if (!ret) {
            LOGE("连接流:%s", url);
            break;
        }
        //记录一个开始时间
        start_time = RTMP_GetTime();
        //表示可以开始推流了
        readyPushing = 1;
        packets.setWork(1);

        RTMPPacket *packet = 0;
        while (readyPushing) {
            packets.pop(packet);
            if (!readyPushing) {
                break;
            }
            if (!packet) {
                continue;
            }
            packet->m_nInfoField2 = rtmp->m_stream_id;
            //发送rtmp包 1：队列
            // 意外断网？发送失败，rtmpdump 内部会调用RTMP_Close
            // RTMP_Close 又会调用 RTMP_SendPacket
            // RTMP_SendPacket  又会调用 RTMP_Close
            // 将rtmp.c 里面WriteN方法的 Rtmp_Close注释掉
            ret = RTMP_SendPacket(rtmp, packet, 1);
            releasePackket(packet);
            if (!ret) {
                LOGE("发送失败");
                break;
            }
        }
        releasePackket(packet);
    } while (0);
    //
    isStart = 0;
    readyPushing = 0;
    packets.setWork(0);
    packets.clear();
    if (rtmp) {
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
    }
    delete (url);
    return 0;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_android_pusher_live_LivePusher_native_1start(JNIEnv *env, jobject instance,
                                                      jstring path_) {
    if (isStart) {
        return;
    }
    isStart = 1;
    const char *path = env->GetStringUTFChars(path_, 0);
    char *url = new char[strlen(path) + 1];
    strcpy(url, path);
    pthread_create(&pid_start, 0, start, url);
    env->ReleaseStringUTFChars(path_, path);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_android_pusher_live_LivePusher_native_1setVideoEncInfo(JNIEnv *env, jobject instance,
                                                                jint width, jint height, jint fps,
                                                                jint bitrate) {
    if (videoChannel) {
        videoChannel->setVideoEncInfo(width, height, fps, bitrate);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_android_pusher_live_LivePusher_native_1pushVideo(JNIEnv *env, jobject instance,
                                                          jbyteArray data_) {
    if (!videoChannel || !readyPushing) {
        return;
    }
    jbyte *data = env->GetByteArrayElements(data_, NULL);
    videoChannel->encodeData(data);
    env->ReleaseByteArrayElements(data_, data, 0);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_android_pusher_live_LivePusher_native_1setAudioEncInfo(JNIEnv *env, jobject instance,
                                                                jint sampleRateInHz,
                                                                jint channels) {

    // TODO

}

extern "C"
JNIEXPORT void JNICALL
Java_com_android_pusher_live_LivePusher_native_1stop(JNIEnv *env, jobject instance) {

    // TODO

}

extern "C"
JNIEXPORT void JNICALL
Java_com_android_pusher_live_LivePusher_native_1release(JNIEnv *env, jobject instance) {
    DELETE(videoChannel);
    DELETE(audioChannel);
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_android_pusher_live_LivePusher_getInputSamples(JNIEnv *env, jobject instance) {
    return 0;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_android_pusher_live_LivePusher_native_1pushAudio(JNIEnv *env, jobject instance,
                                                          jbyteArray data_) {
    jbyte *data = env->GetByteArrayElements(data_, NULL);


    env->ReleaseByteArrayElements(data_, data, 0);
}