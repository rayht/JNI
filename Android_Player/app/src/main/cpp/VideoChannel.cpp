/**
 * @Author Leiht
 * @Date 2018/10/2
 */

#include "VideoChannel.h"
#include "macro.h"

/**
 * 丢包，处理AVPacket
 * 注意：不能直接丢I帧，如果要丢I帧就必须丢掉其后面的所有B帧和P帧
 * @param q
 */
void dropAvPacket(queue<AVPacket*> &q) {
    while (!q.empty()) {
        AVPacket *packet = q.front();
        //如果不属于 I 帧
        if (packet->flags != AV_PKT_FLAG_KEY) {
            BaseChannel::releaseAvPacket(&packet);
            q.pop();
        } else {
            break;
        }
    }
}

/**
 * 丢已经解码的图片
 * @param q
 */
void dropAvFrame(queue<AVFrame*> &q) {
    if (!q.empty()) {
        AVFrame *frame = q.front();
        BaseChannel::releaseAvFrame(&frame);
        q.pop();
    }
}

VideoChannel::VideoChannel(int id, AVCodecContext *avCodecContext, AVRational time_base, int fps, JavaCallHelper *javaCallHelper) :
        BaseChannel(id, avCodecContext, time_base, javaCallHelper) {
    this->fps = fps;
    /**
     * 丢包，设置丢掉SafeQueue里的同步方法
     */
    //1. 丢packets队列
    //    packets.setSyncHandle(dropAvPacket);
    //2. 丢frames队列
    frames.setSyncHandle(dropAvFrame);
}

VideoChannel::~VideoChannel() {
    frames.clear();
    packets.clear();
    if (avCodecContext) {
        avcodec_close(avCodecContext);
        avcodec_free_context(&avCodecContext);
        avCodecContext = 0;
    }
}

void* video_decode_task(void *args) {
    VideoChannel *videoChannel = static_cast<VideoChannel *>(args);
    videoChannel->decode();
    return 0;
}

void* render_task(void *args) {
    VideoChannel *videoChannel = static_cast<VideoChannel *>(args);
    videoChannel->render();
    return 0;
}

void VideoChannel::play() {
    //设置状态为播放状态
    isPlaying = 1;

    //设置SafeQueue packets为工作状态
    packets.setWork(1);
    //设置SafeQueue frames为工作状态
    frames.setWork(1);

    /**
     * 启动解码线程
     */
    pthread_create(&pid_decode, 0, video_decode_task, this);

    /**
     * 启动播放线程
     */
    pthread_create(&pid_render, 0, render_task, this);
}

void VideoChannel::decode() {
    AVPacket *packet = 0;
    while (isPlaying) {
        int ret = packets.pop(packet);
        if(!isPlaying) {
            if(packet) {
                releaseAvPacket(&packet);
            }
            break;
        }
        //取出失败
        if(!ret) {
            continue;
        }

        //把取出的packet传入ffmpeg的解码器
        ret = avcodec_send_packet(avCodecContext, packet);
        releaseAvPacket(&packet);
        //重试
        if(ret != 0) {
            continue;
        }

        //AVFrame,一个帧，代表了一个图像 (将这个图像先输出来)
        AVFrame *frame = av_frame_alloc();
        //从解码器中读取 解码后的数据包 AVFrame
        ret = avcodec_receive_frame(avCodecContext, frame);
        if (ret == AVERROR(EAGAIN)) {
            //需要更多的数据才能够进行解码
            continue;
        } else if (ret != 0) {
            break;
        }

        //放入frames队列，播放线程等待读取
        frames.push(frame);
    }
    releaseAvPacket(&packet);
}

void VideoChannel::render() {
    /**
     * 获取SwsContext
     * srcW -> avCodecContext->width 原frame的宽
     * srcH -> avCodecContext->height 原frame的高
     * enum AVPixelFormat -> avCodecContext->pix_fmt 转换原格式
     * dstW -> avCodecContext->width 转换目标格式的宽
     * dstH -> avCodecContext->height 转换目标格式的高
     * enum AVPixelFormat -> AV_PIX_FMT_RGBA 目标格式
     * flags -> SWS_BILINEAR 指定用于重新缩放的算法和选项
     */
    swsContext = sws_getContext(avCodecContext->width,
                                avCodecContext->height,
                                avCodecContext->pix_fmt,
                                avCodecContext->width,
                                avCodecContext->height,
                                AV_PIX_FMT_RGBA,
                                SWS_BILINEAR, 0, 0, 0);

    AVFrame *frame = 0;

    //指针数组
    uint8_t *dst_data[4];
    //每一行保存多少个数据
    int dst_linesize[4];

    //给dst_data dst_linesize申请内存
    av_image_alloc(dst_data, dst_linesize,
                   avCodecContext->width, avCodecContext->height, AV_PIX_FMT_RGBA, 1);
    while(isPlaying) {
        int ret = frames.pop(frame);
        if(!isPlaying) {
            break;
        }
        //数据格式转换
        sws_scale(swsContext, reinterpret_cast<const uint8_t *const *>(frame->data),
                  frame->linesize, 0,
                  avCodecContext->height,
                  dst_data,
                  dst_linesize);
        //fps,帧率，即每秒显示多少张图片，1.0 / fps即两张图片之间间隔的标准时间
        //如帧率为25，时间为1.0/25s,0.4s为图片之间显示的间隔时间
        double frame_delays = 1.0 / fps;
        //获取当前帧的播放的延迟时间(每帧都有这个相对的延迟时间，否则视频播放会很快,音频数据也有相同的时间延迟)
        double clock = frame->best_effort_timestamp * av_q2d(time_base);
        //额外的间隔时间
        double extra_delay = frame->repeat_pict / (2 * fps);
        // 真实需要的间隔时间
        double delays = extra_delay + frame_delays;
        if(!audioChannel) {
            //只播放视频，没有音频不作同步处理
            //只sleep标准时间
            av_usleep(delays * 1000000);
        }else {
            if (clock == 0) {
                av_usleep(delays * 1000000);
            } else {
                //比较音频与视频的相对播放时间clock的时间差
                double audio_clock = audioChannel->clock;
                //视频的相对时间-音频的相对时间差
                //diff 大于0则说明视频播放快，<0则说明音频播放快
                double diff = clock - audio_clock;
                if (diff >= 0) {
                    LOGE("视频快了或者时间相等:diff=%lf", diff);
                    if (diff > 1) {
                        //差的太久了， 那只能慢慢赶 不然就是卡好久
                        av_usleep((delays * 2) * 1000000);
                    } else {
                        //差的不多，尝试一次赶上去
                        av_usleep((delays + diff) * 1000000);
                    }
                } else {
                    LOGE("音频快了：%lf", diff);
                    if (fabs(diff) >= 0.05) {
                        //时间差大于等于0.05，差值太大，需要丢包
                        //释放当前frame,不显示当前帧
                        releaseAvFrame(&frame);
                        frames.sync();
                        continue;
                    } else {
                        //不处理， 快点赶上 音频
                    }
                }
            }
        }

        //diff太大了不回调了
        if (javaCallHelper && !audioChannel) {
            javaCallHelper->onProgress(THREAD_CHILD, clock);
        }

        //回调出去进行播放,数据都存在数组的第一个元素上
        renderFrameCallback(dst_data[0], dst_linesize[0], avCodecContext->width, avCodecContext->height);
        releaseAvFrame(&frame);
    }
    av_freep(&dst_data[0]);
    releaseAvFrame(&frame);
    isPlaying = 0;
    sws_freeContext(swsContext);
    swsContext = 0;
}

void VideoChannel::stop() {
    isPlaying = 0;
    frames.setWork(0);
    packets.setWork(0);
    pthread_join(pid_decode, 0);
    pthread_join(pid_render, 0);
}

void VideoChannel::setRenderFrameCallback(RenderFrameCallback renderFrameCallback) {
    this->renderFrameCallback = renderFrameCallback;
}

void VideoChannel::setAudioChannel(AudioChannel *audioChannel) {
    this->audioChannel = audioChannel;
}
