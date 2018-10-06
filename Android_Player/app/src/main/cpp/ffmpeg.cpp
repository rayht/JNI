/**
 * @Author:Leiht
 * @Date:2018/10/1
 */

#include <cstring>
#include <pthread.h>
#include "ffmpeg.h"
#include "macro.h"

Ffmpeg::Ffmpeg(JavaCallHelper *javaCallHelper, const char *dataSource) {
    //赋数据源地址
    url = new char[strlen(dataSource) + 1];
    strcpy(url, dataSource);

    this->javaCallHelper = javaCallHelper;

    isPlaying = false;
    duration = 0;

    pthread_mutex_init(&seekMutex, 0);
}

Ffmpeg::~Ffmpeg() {
    pthread_mutex_destroy(&seekMutex);
    DELETE(url);
}

void *task_prepare(void *args) {
    Ffmpeg *ffmpeg = static_cast<Ffmpeg *>(args);
    ffmpeg->_prepare();
    return 0;
}

void Ffmpeg::prepare() {
    //开启线程,在线程里面调用Ffmpeg._prepare()方法
    pthread_create(&pid_prepare, NULL, task_prepare, this);
}

void Ffmpeg::_prepare() {
    /**
     * 打开ffmpeg网络访问(需要在释放的时候调用avformat_network_deinit()方法)
     */
    avformat_network_init();

    /**
     * 1. 注册所有ffmpeg组件，在4.x版本以上可以忽略该方法
     */
    //av_register_all();

    /**
     * 2. 打开流媒体(文件地址/直播流地址)
     * 参数：AVFormatContext ：包含媒体信息，视频宽高等
     *      url : 流媒体地址
     *      AVInputFormat ： 指示打开的媒体格式(传NULL，ffmpeg就会自动推到出是mp4还是flv)
     *      AVDictionary ：设定参数，如超时时间等
     */
    AVDictionary *options = 0;
    //设置超时时间 微妙 超时时间5秒
    av_dict_set(&options, "timeout", "5000000", 0);
    int ret = avformat_open_input(&formatContext, url, 0, &options);
    av_dict_free(&options);
    if (ret != 0) {
        LOGE("打开流媒体文件失败：%s", av_err2str(ret));
        if (javaCallHelper) {
            javaCallHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_OPEN_URL);
        }
        return;
    }

    /**
     * 2. 查找媒体中的音视流(给Context中的streams等成员赋值)
     */
    ret = avformat_find_stream_info(formatContext, 0);
    if (ret < 0) {
        LOGE("查找媒体流失败：%s", av_err2str(ret));
        if (javaCallHelper) {
            javaCallHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_FIND_STREAMS);
        }
        return;
    }

    //视频时长（单位：微秒us，转换为秒需要除以1000000）
    duration = formatContext->duration / 1000000;

    /**
     * 3. 循环获取流(包括音视频流)，作分别处理
     */
    for (int i = 0; i < formatContext->nb_streams; ++i) {
        //nb_streams相当于读取流媒体文件的开头
        // 3.1. 获取些流里包含什么流媒体文件，目前一般有音频和视频流，也就是nb_streams长度为2
        AVStream *avStream = formatContext->streams[i];

        //AVCodecParameters包含了解码这段流的各种参数信息(宽、高、码率、帧率等)
        AVCodecParameters *codecPar = avStream->codecpar;
        //3.2 获取解码器(通过当前流使用的编码方式AVCodecID codecId = codecPar->codec_id)
        AVCodec *dec = avcodec_find_decoder(codecPar->codec_id);
        if (dec == NULL) {
            LOGE("获取解码器失败");
            if (javaCallHelper) {
                javaCallHelper->onError(THREAD_CHILD, FFMPEG_FIND_DECODER_FAIL);
            }
            return;
        }

        //3.3 获取解码器上下文
        AVCodecContext *codecContext = avcodec_alloc_context3(dec);
        if (codecContext == NULL) {
            LOGE("获取解码器上下文失败");
            if (javaCallHelper) {
                javaCallHelper->onError(THREAD_CHILD, FFMPEG_ALLOC_CODEC_CONTEXT_FAIL);
            }
            return;
        }

        /**
         * 3.4. 设置上下文内的一些参数 (context->width)
         * context->width = codecpar->width;
         * context->height = codecpar->height;
         */
        ret = avcodec_parameters_to_context(codecContext, codecPar);
        if (ret < 0) {
            LOGE("设置解码器上下文参数失败:%s", av_err2str(ret));
            if (javaCallHelper) {
                javaCallHelper->onError(THREAD_CHILD, FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL);
            }
            return;
        }

        //3.5. 打开解码器
        ret = avcodec_open2(codecContext, dec, 0);
        if (ret != 0) {
            LOGE("打开解码器失败:%s", av_err2str(ret));
            if (javaCallHelper) {
                javaCallHelper->onError(THREAD_CHILD, FFMPEG_OPEN_DECODER_FAIL);
            }
            return;
        }

        AVRational avRational = avStream->time_base;
        /**
         * 3.6 根据流类型创建对应的Channel
         */
        if (codecPar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioChannel = new AudioChannel(i, codecContext, avRational, javaCallHelper);
        } else if (codecPar->codec_type == AVMEDIA_TYPE_VIDEO) {
            //帧率： 单位时间内 需要显示多少个图像
            AVRational frame_rate = avStream->avg_frame_rate;
            //自己除也可以->int fps = frame_rate.num / frame_rate.den;
            int fps = av_q2d(frame_rate);
            videoChannel = new VideoChannel(i, codecContext, avRational, fps, javaCallHelper);
            videoChannel->setRenderFrameCallback(this->renderFrameCallback);
        }
    }

    if (videoChannel == NULL && audioChannel == NULL) {
        LOGE("没有音视频流");
        if (javaCallHelper) {
            javaCallHelper->onError(THREAD_CHILD, FFMPEG_NOMEDIA);
        }
        return;
    }

    /**
     * 3.7 准备完成，通知Java可以播放
     */
    if (javaCallHelper) {
        javaCallHelper->onParpare(THREAD_CHILD);
    }
}

void *play(void *args) {
    Ffmpeg *ffmpeg = static_cast<Ffmpeg *>(args);
    ffmpeg->_start();
    return 0;
}

void Ffmpeg::start() {
    //正在播放
    isPlaying = 1;

    /**
     * 播放视频
     */
    if (videoChannel) {
        videoChannel->setAudioChannel(audioChannel);
        videoChannel->play();
    }

    /**
     * 播放音频
     */
    if (audioChannel) {
        audioChannel->play();
    }

    /**
     * 启动读取流(包括音视流)的线程
     */
    pthread_create(&pid_play, 0, play, this);
}

/**
 * 读取媒体流数据包(包含音视频包)
 */
void Ffmpeg::_start() {
    int ret;
    while (isPlaying) {
        //读取文件的时候没有网络请求，一下子读完了，可能导致oom
        //特别是读本地文件的时候 一下子就读完了
        if (audioChannel && audioChannel->packets.size() > 100) {
            //10ms
            av_usleep(1000 * 10);
            continue;
        }
        if (videoChannel && videoChannel->packets.size() > 100) {
            av_usleep(1000 * 10);
            continue;
        }

        //同步，防止seek与读取同时操作
        pthread_mutex_lock(&seekMutex);
        //读取包
        AVPacket *packet = av_packet_alloc();
        // 从媒体中读取音频、视频包
        ret = av_read_frame(formatContext, packet);
        pthread_mutex_unlock(&seekMutex);

        if (ret == 0) {
            //读取成功
            if (audioChannel && packet->stream_index == audioChannel->id) {
                audioChannel->packets.push(packet);
            } else if (videoChannel && packet->stream_index == videoChannel->id) {
                videoChannel->packets.push(packet);
            }
        } else if (ret == AVERROR_EOF) {
            //读取完成 但是可能还没播放完
            if (audioChannel->packets.empty() && audioChannel->frames.empty()
                && videoChannel->packets.empty() && videoChannel->frames.empty()) {
                break;
            }
            //为什么这里要让它继续循环 而不是sleep
            //如果是做直播 ，可以sleep
            //如果要支持点播(播放本地文件） seek 后退
        } else {
            break;
        }
    }
    isPlaying = 0;
    if (audioChannel) {
        audioChannel->stop();
    }
    if (videoChannel) {
        videoChannel->stop();
    }
}

void Ffmpeg::setRenderFrameCallback(RenderFrameCallback renderFrameCallback) {
    this->renderFrameCallback = renderFrameCallback;
}

void *sync_stop(void *args) {
    Ffmpeg *ffmpeg = static_cast<Ffmpeg *>(args);

    //等待两个线程执行完成
    pthread_join(ffmpeg->pid_prepare, 0);
    pthread_join(ffmpeg->pid_play, 0);

    DELETE(ffmpeg->videoChannel);
    DELETE(ffmpeg->audioChannel);

    // 这时候释放就不会出现问题了
    if (ffmpeg->formatContext) {
        //先关闭读取 (关闭fileintputstream)
        avformat_close_input(&ffmpeg->formatContext);
        avformat_free_context(ffmpeg->formatContext);
        ffmpeg->formatContext = 0;
    }
    DELETE(ffmpeg);

    return 0;
}

void Ffmpeg::stop() {
    isPlaying = 0;
    javaCallHelper = 0;

    pthread_create(&pid_stop, 0, sync_stop, this);
    pthread_join(pid_stop, 0);
    avformat_network_deinit();
}

int Ffmpeg::getDuration() {
    return duration;
}

void Ffmpeg::seek(int i) {
    //进去必须 在0- duration 范围之类
    if (i< 0 || i >= duration) {
        return;
    }
    if (!audioChannel && !videoChannel) {
        return;
    }
    if (!formatContext) {
        return;
    }
    isSeek = 1;
    pthread_mutex_lock(&seekMutex);
    //单位是 微妙
    int64_t seek = i * 1000000;
    //seek到请求的时间 之前最近的关键帧
    // 只有从关键帧才能开始解码出完整图片
    av_seek_frame(formatContext, -1,seek, AVSEEK_FLAG_BACKWARD);
//    avformat_seek_file(formatContext, -1, INT64_MIN, seek, INT64_MAX, 0);
    // 音频、与视频队列中的数据 是不是就可以丢掉了？
    if (audioChannel) {
        //暂停队列
        audioChannel->stopWork();
        //可以清空缓存
//        avcodec_flush_buffers();
        audioChannel->clear();
        //启动队列
        audioChannel->startWork();
    }
    if (videoChannel) {
        videoChannel->stopWork();
        videoChannel->clear();
        videoChannel->startWork();
    }
    pthread_mutex_unlock(&seekMutex);
    isSeek = 0;
}
