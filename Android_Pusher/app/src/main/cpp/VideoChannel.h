/**
 * @Author Leiht
 * @Date 2018/10/6
 */
#ifndef ANDROID_PUSHER_VIDEOCHANNEL_H
#define ANDROID_PUSHER_VIDEOCHANNEL_H

#include <inttypes.h>
#include <pthread.h>
#include "librtmp/rtmp.h"
#include "include/x264.h"

typedef void (*VideoCallback)(RTMPPacket *packet);

class VideoChannel {
private:
    pthread_mutex_t mutex;
    int mWidth;
    int mHeight;
    int mFps;
    int mBitrate;
    x264_t *videoCodec = 0;
    x264_picture_t *pic_in = 0;

    int ySize;
    int uvSize;
    VideoCallback videoCallback;
public:
    VideoChannel();

    ~VideoChannel();

    void setVideoCallback(VideoCallback videoCallback);

    void setVideoEncInfo(int width, int height, int fps, int bitrate);

    void sendSpsPps(uint8_t *sps, uint8_t *pps, int sps_len, int pps_len);

    void sendFrame(int type, uint8_t *payload, int i_payload);

    void encodeData(int8_t *data);

};

#endif //ANDROID_PUSHER_VIDEOCHANNEL_H
