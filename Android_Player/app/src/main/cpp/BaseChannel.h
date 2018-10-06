/**
 * @Author Leiht
 * @Date 2018/10/2
 */

#ifndef ANDROID_PLAYER_BASECHANNEL_H
#define ANDROID_PLAYER_BASECHANNEL_H

#include "safe_queue.h"
#include "macro.h"
#include "JavaCallHelper.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libavutil/time.h>
};

class BaseChannel {
private:

public:
    /**
     * 对应nbStream的id
     */
    int id;

    /**
     * 编码数据包队列
     */
    SafeQueue<AVPacket *> packets;

    /**
     * 解码数据包队列
     */
    SafeQueue<AVFrame *> frames;

    SwsContext *swsContext = 0;

    bool isPlaying;

    AVCodecContext *avCodecContext;

    //时间戳单位
    AVRational time_base;

    JavaCallHelper *javaCallHelper;

    BaseChannel(int id, AVCodecContext *avCodecContext, AVRational time_base, JavaCallHelper *javaCallHelper) : id(id), avCodecContext(avCodecContext), time_base(time_base), javaCallHelper(javaCallHelper) {
        packets.setReleaseCallback(releaseAvPacket);
        frames.setReleaseCallback(releaseAvFrame);
    }
    //虚函数，子类在析构方法中才能调用父类的析构方法
    virtual ~BaseChannel() {
        if (avCodecContext) {
            avcodec_close(avCodecContext);
            avcodec_free_context(&avCodecContext);
            avCodecContext = 0;
        }
        packets.clear();
        frames.clear();
        LOGE("释放channel:%d %d", packets.size(), frames.size());
    }

    /**
     * 纯虚方法 相当于 抽象方法
     */
    virtual void play() = 0;

    virtual void stop() = 0;

    void clear() {
        packets.clear();
        frames.clear();
    }

    void stopWork() {
        packets.setWork(0);
        frames.setWork(0);
    }

    void startWork() {
        packets.setWork(1);
        frames.setWork(1);
    }

    /**
     * 释放 AVPacket
     * @param packet
    */
    static void releaseAvPacket(AVPacket **packet) {
        if (packet) {
            av_packet_free(packet);
            //为什么用指针的指针？指针的指针能够修改传递进来的指针的指向
            *packet = 0;
        }
    }

    static void releaseAvFrame(AVFrame **frame) {
        if (frame) {
            av_frame_free(frame);
            //为什么用指针的指针？
            // 指针的指针能够修改传递进来的指针的指向
            *frame = 0;
        }
    }
};

#endif //ANDROID_PLAYER_BASECHANNEL_H
