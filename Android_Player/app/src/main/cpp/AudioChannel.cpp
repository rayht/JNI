/**
 * @Author Leiht
 * @Date 2018/10/2
 */

#include "AudioChannel.h"
#include "macro.h"

AudioChannel::AudioChannel(int id, AVCodecContext *avCodecContext, AVRational time_base, JavaCallHelper *javaCallHelper) :
        BaseChannel(id, avCodecContext, time_base, javaCallHelper) {
    /**
     * 输出声道，此处为双声道
     */
    out_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    /**
     * 采样位音频格式
     * AV_SAMPLE_FMT_S16 ： 16位 2字节
     */
    out_samplesize = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
    /**
     * 输出采样率
     * 44100, 为通用采样率，在所有设备上都可以正常工作
     * 请参考： AudioRecord.java
     * the sample rate expressed in Hertz. 44100Hz is currently the only
     * rate that is guaranteed to work on all devices, but other rates such as 22050,
     * 16000, and 11025 may work on some devices.
     */
    out_sample_rate = 44100;
    /**
     * 44100个16位 44100 * 2 44100*(双声道)*(16位)
     * 分配内存
     */
    data = static_cast<uint8_t *>(malloc(out_sample_rate * out_channels * out_samplesize));
    memset(data, 0, out_sample_rate * out_channels * out_samplesize);
}

AudioChannel::~AudioChannel() {
    if (swrContext) {
        swr_free(&swrContext);
        swrContext = 0;
    }
    DELETE(data);
}

/**
 * 解码线程方法
 * @param args
 * @return
 */
void *audio_decode_task(void *args) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(args);
    audioChannel->decode();
    return 0;
}

/**
 * 播放线程方法
 * @param args
 * @return
 */
void *play_task(void *args) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(args);
    audioChannel->_play();
    return 0;
}

void AudioChannel::play() {
    isPlaying = 1;

    //设置packets frames工作状态
    packets.setWork(1);
    frames.setWork(1);

    /**
     * 重采样 SwrContext
     * SwrContext -> 0, 这里传空，接收返回值
     * AV_CH_LAYOUT_STEREO 输出双声道
     * AV_SAMPLE_FMT_S16 输出采样位
     * out_sample_rate 输出采样率，44100
     * avCodecContext->channel_layout 输入声道
     * avCodecContext->sample_fmt 输入采样位
     * avCodecContext->sample_rate 输入采样率，44100
     */
    swrContext = swr_alloc_set_opts(0, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, out_sample_rate,
                                    avCodecContext->channel_layout, avCodecContext->sample_fmt,
                                    avCodecContext->sample_rate, 0, 0);
    //初始化
    swr_init(swrContext);

    /**
     * 解码
     */
    pthread_create(&pid_audio_decode, 0, audio_decode_task, this);
    /**
     * 播放
     */
    pthread_create(&pid_audio_play, 0, play_task, this);
}

/**
 * 解码方法
 */
void AudioChannel::decode() {
    AVPacket *packet = 0;
    while (isPlaying) {
        //取出一个音频数据包
        int ret = packets.pop(packet);
        if (!isPlaying) {
            break;
        }
        //取出失败
        if (!ret) {
            continue;
        }
        //把包传入解码器
        ret = avcodec_send_packet(avCodecContext, packet);
        releaseAvPacket(&packet);
        //重试
        if (ret != 0) {
            continue;
        }
        //音频frame
        AVFrame *frame = av_frame_alloc();
        //从解码器中读取 解码后的数据包 AVFrame
        ret = avcodec_receive_frame(avCodecContext, frame);
        //需要更多的数据才能够进行解码
        if (ret == AVERROR(EAGAIN)) {
            continue;
        } else if (ret != 0) {
            break;
        }
        while (frames.size() > 100 && isPlaying) {
            releaseAvPacket(&packet);
            av_usleep(1000 * 10);
            continue;
        }
        //将frame传入frames，在播放线程取出frame播放(流畅度)
        frames.push(frame);
    }
    releaseAvPacket(&packet);
}

//返回获取的pcm数据大小
int AudioChannel::getPcm() {
//    int data_size = 0;
//    AVFrame *frame;
//    int ret = frames.pop(frame);
//    if (!isPlaying) {
//        if (ret) {
//            releaseAvFrame(&frame);
//        }
//        return data_size;
//    }
//
//    /**
//     * 重采样
//     */
//    //48000HZ 8位 -> 44100 16位
//    // 假设我们输入了10个数据 ，swrContext转码器 这一次处理了8个数据
//    // 那么如果不加delays(上次没处理完的数据) , 积压导致内存泄漏
//    int64_t delays = swr_get_delay(swrContext, frame->sample_rate);
//    // max_samples 将 nb_samples 个数据 由 sample_rate采样率转成 44100 后 返回多少个数据
//    // 10  个 48000 = nb 个 44100
//    // AV_ROUND_UP : 向上取整 1.1 = 2
//    //delays+frame->nb_samples 上次剩余+当前帧数据
//    int64_t max_samples = av_rescale_rnd(delays + frame->nb_samples, out_sample_rate,
//                                         frame->sample_rate, AV_ROUND_UP);
//    //上下文+输出缓冲区+输出缓冲区能接受的最大数据量+输入数据+输入数据个数
//    //返回 每一个声道的输出数据实际个数
//    int samples = swr_convert(swrContext, &data, max_samples, (const uint8_t **) frame->data,
//                              frame->nb_samples);
//    //获得   samples 个   * 2 声道 * 2字节（16位）
//    data_size = samples * out_samplesize * out_channels;
//
//    /**
//     * 获取 frame 的一个相对播放时间 （相对开始播放）
//     * 得 相对播放这一段数据的秒数
//     * 用于音视频同步时与视频的这个时间作比较从而实现音视频同步
//     */
//    clock = frame->pts * av_q2d(time_base);
//    if (javaCallHelper) {
//        javaCallHelper->onProgress(THREAD_CHILD, clock);
//    }
//
//    releaseAvFrame(&frame);
//    return data_size;

    int data_size = 0;
    AVFrame *frame = 0;
    while (isPlaying) {
        int ret = frames.pop(frame);
        if (!isPlaying) {
            break;
        }
        if (!ret) {
            continue;
        }
        // 计算转换后的sample个数  类似 a * b / c
        // swr_get_delay： 延迟时间:  输入了10个 数据，可能这次转换只转换了8个数据，那么还剩余2个 这一个函数就是得到上一次剩余的这个2
        // av_rescale_rnd： 以3为单位的1 转为以2为单位
        // 10个2=20
        // 转成 以4 为单位
        // 10*4/2
        uint64_t dst_nb_samples = av_rescale_rnd(
                swr_get_delay(swrContext, frame->sample_rate) + frame->nb_samples,
                out_sample_rate,
                frame->sample_rate,
                AV_ROUND_UP);
        // 转换，返回值为转换后的sample个数
        int nb = swr_convert(swrContext, &data, dst_nb_samples,
                             (const uint8_t **) frame->data, frame->nb_samples);
        //转换后多少数据
        data_size = nb * out_channels * out_samplesize;
        //音频的时间
        clock = frame->best_effort_timestamp * av_q2d(time_base);
        if (javaCallHelper) {
            javaCallHelper->onProgress(THREAD_CHILD, clock);
        }
        break;
    }
    releaseAvFrame(&frame);
    return data_size;
}

void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(context);
    //获得pcm 数据 多少个字节 data
    int dataSize = audioChannel->getPcm();
    if (dataSize > 0) {
        // 接收16位数据
        (*bq)->Enqueue(bq, audioChannel->data, dataSize);
    }
}

/**
 * 播放方法
 */
void AudioChannel::_play() {
    /**
     * 1. 创建OpenSL ES引擎并获取引擎接口
     */
    SLresult result;
    //1.1 创建引擎 SLObjectItf engineObject
    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    if (SL_RESULT_SUCCESS != result) {
        LOGE("创建OpenSL ES引擎错误，音频播放失败.");
        return;
    }
    //1.2 初始化引擎
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        LOGE("引擎初始化错误，音频播放失败.");
        return;
    }
    //1.3 获取引擎接口
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineInterface);
    if (SL_RESULT_SUCCESS != result) {
        LOGE("获取引擎接口错误，音频播放失败.");
        return;
    }

    /**
     * 2. 设置混音器
     */
    //2.1 创建混音器SLObjectItf outputMixObject
    result = (*engineInterface)->CreateOutputMix(engineInterface, &outputMixObject, 0, 0, 0);
    if (SL_RESULT_SUCCESS != result) {
        LOGE("创建混音器错误，音频播放失败.");
        return;
    }

    //2.2 初始化混音器
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        LOGE("初始化混音器错误，音频播放失败.");
        return;
    }

    /**
     * 3. 创建播放器
     */
    //3.1 配置输入声音信息 创建buffer缓冲类型的队列 2个队列
    SLDataLocator_AndroidSimpleBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                            2};
    /**
     * pcm数据格式
     * SL_DATAFORMAT_PCM PCM 数据格式
     * 2 ：双声道
     * SL_SAMPLINGRATE_44_1 采样率，44100
     * AV_SAMPLE_FMT_S16 采样位 16byte->2字节
     * 16(数据的大小) ?
     * LEFT|RIGHT(双声道)
     * 小端数据
     */
    SLDataFormat_PCM pcm = {SL_DATAFORMAT_PCM, 2, SL_SAMPLINGRATE_44_1, SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
                            SL_BYTEORDER_LITTLEENDIAN};
    //数据源(将上述配置信息放到这个数据源中)
    SLDataSource slDataSource = {&android_queue, &pcm};

    //3.2  配置音轨(输出)
    //设置混音器
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&outputMix, NULL};
    //需要的接口  操作队列的接口
    const SLInterfaceID ids[1] = {SL_IID_BUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};

    //3.3 创建播放器
    (*engineInterface)->CreateAudioPlayer(engineInterface, &bqPlayerObject, &slDataSource,
                                          &audioSnk, 1,
                                          ids, req);

    //3.4 初始化播放器
    (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);

    //得到接口后调用  获取Player接口
    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerInterface);

    /**
     * 4、设置播放回调函数
     */
    //获取播放器队列接口
    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE,
                                    &bqPlayerBufferQueueInterface);
    //设置回调
    (*bqPlayerBufferQueueInterface)->RegisterCallback(bqPlayerBufferQueueInterface,
                                                      bqPlayerCallback, this);
    /**
     * 5、设置播放状态
     */
    (*bqPlayerInterface)->SetPlayState(bqPlayerInterface, SL_PLAYSTATE_PLAYING);
    /**
     * 6、手动激活一下这个回调
     */
    bqPlayerCallback(bqPlayerBufferQueueInterface, this);
}

void AudioChannel::stop() {
    isPlaying = 0;

    packets.setWork(0);
    frames.setWork(0);

    pthread_join(pid_audio_decode, 0);
    pthread_join(pid_audio_play, 0);

    releaseOpenSL();

    if (swrContext) {
        swr_free(&swrContext);
        swrContext = 0;
    }
}

/**
 * 释放OpenSL ES
 */
void AudioChannel::releaseOpenSL() {
    //设置停止状态
    if (bqPlayerInterface) {
        (*bqPlayerInterface)->SetPlayState(bqPlayerInterface, SL_PLAYSTATE_STOPPED);
        bqPlayerInterface = 0;
    }
    //销毁播放器
    if (bqPlayerObject) {
        (*bqPlayerObject)->Destroy(bqPlayerObject);
        bqPlayerObject = 0;
        bqPlayerBufferQueueInterface = 0;
    }
    //销毁混音器
    if (outputMixObject) {
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = 0;
    }
    //销毁引擎
    if (engineObject) {
        (*engineObject)->Destroy(engineObject);
        engineObject = 0;
        engineInterface = 0;
    }
}
