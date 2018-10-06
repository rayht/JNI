/**
 * @Author Leiht
 * @Date 2018/10/2
 */

#ifndef ANDROID_PLAYER_AUDIOCHANNEL_H
#define ANDROID_PLAYER_AUDIOCHANNEL_H

#include "BaseChannel.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

class AudioChannel : public BaseChannel {
private:
    pthread_t pid_audio_decode;
    pthread_t pid_audio_play;

    //重采样
    SwrContext *swrContext = 0;

    int out_channels;
    int out_samplesize;
    int out_sample_rate;

    // OpenSL ES引擎对象
    SLObjectItf engineObject = 0;
    // OpenSL ES引擎接口
    SLEngineItf engineInterface = 0;
    //混音器
    SLObjectItf outputMixObject = 0;
    //播放器
    SLObjectItf bqPlayerObject = 0;
    //播放器接口
    SLPlayItf bqPlayerInterface = 0;
    //队列结构
    SLAndroidSimpleBufferQueueItf bqPlayerBufferQueueInterface = 0;
public:
    uint8_t *data = 0;

    double clock;
public:
    AudioChannel(int id, AVCodecContext *avCodecContext, AVRational time_base, JavaCallHelper *javaCallHelper);

    ~AudioChannel();

    void play();

    void decode();

    void _play();

    void stop();

    int getPcm();

    void releaseOpenSL();
};

#endif //ANDROID_PLAYER_AUDIOCHANNEL_H
