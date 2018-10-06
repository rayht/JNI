/**
 * @Author Leiht
 * @Date 2018/10/2
 */

#ifndef ANDROID_PLAYER_VIDEOCHANNEL_H
#define ANDROID_PLAYER_VIDEOCHANNEL_H

#include "BaseChannel.h"
#include "AudioChannel.h"

/**
 * 1、解码
 * 2、播放
 */
typedef void (*RenderFrameCallback)(uint8_t *, int, int, int);

class VideoChannel : public BaseChannel {
private:
    pthread_t pid_decode;
    pthread_t pid_render;

    RenderFrameCallback renderFrameCallback;

    int fps;

    AudioChannel *audioChannel;
public:
    VideoChannel(int id, AVCodecContext *avCodecContext, AVRational time_base, int fps, JavaCallHelper *javaCallHelper);

    ~VideoChannel();

    /**
     * 播放
     */
    void play();

    /**
     * 解码
     */
    void decode();

    /**
     * 渲染，显示到SurfaceView
     */
    void render();

    void stop();

    void setRenderFrameCallback(RenderFrameCallback renderFrameCallback);

    void setAudioChannel(AudioChannel *audioChannel);
};

#endif //ANDROID_PLAYER_VIDEOCHANNEL_H
