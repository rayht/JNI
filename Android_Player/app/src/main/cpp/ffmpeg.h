/**
 * @Author:Leiht
 * @Date:2018/10/1
 */
#ifndef ANDROID_PLAYER_FFMPEG_H
#define ANDROID_PLAYER_FFMPEG_H

#include "JavaCallHelper.h"
#include "VideoChannel.h"
#include "AudioChannel.h"

extern "C" {
#include <libavformat/avformat.h>
}

class Ffmpeg {
public:
    char *url;

    JavaCallHelper *javaCallHelper;

    pthread_t pid_prepare;
    pthread_t pid_play;
    pthread_t pid_stop;

    pthread_mutex_t seekMutex;

    AVFormatContext *formatContext = 0;

    VideoChannel *videoChannel = 0;

    AudioChannel *audioChannel = 0;

    RenderFrameCallback renderFrameCallback = 0;


    bool isPlaying;
    int duration;
    bool isSeek = 0;
public:
    Ffmpeg(JavaCallHelper *javaCallHelper, const char *dataSource);

    ~Ffmpeg();

    /**
     * prepare，打开ffmpeg组件
     */
    void prepare();
    void _prepare();

    /**
     * 读取AvPacket放入队列
     */
    void start();
    void _start();

    void stop();

    void setRenderFrameCallback(RenderFrameCallback renderFrameCallback);

    int getDuration();

    void seek(int i);
};

#endif //ANDROID_PLAYER_FFMPEG_H
