/**
 * 查看文件类型：./ffplay -f rawvideo -s 480x480 /Users/zj-db0519/Desktop/material/folder/video/100f2.yuv
 */


#include "HBFilterWater.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavfilter/avfiltergraph.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"
#include <SDL.h>
#include <SDL_video.h>

// SDL 的开关
#define ENABLE_SDL 0
// 用YUV 数据输出
#define ENABLE_YUVFILE 1

const char *filter_descr = "movie=/Users/zj-db0519/Desktop/material/folder/mltframework_test/watermark1.png[wm];[in][wm]overlay=5:5[out]";

static AVFormatContext* pFormatCtx = NULL;
static AVCodecContext * pCodecCtx = NULL;
AVFilterContext *buffersink_ctx = NULL;
AVFilterContext *buffersrc_ctx = NULL;
AVFilterGraph   *filter_graph = NULL;
static int video_stream_index = -1;

static int open_input_file(const char *filename)
{
    int ret = -1;
    AVCodec *dec = NULL;
    
    /* @avformat_open_input： 打开媒体
     * 1、输入输出结构体AVIOContext的初始化；2、输入数据的协议（例如RTMP，或者file）的识别（通过一套评分机制）:1判断文件名的后缀 2读取文件头的数据进行比对；
     * 3、使用获得最高分的文件协议对应的URLProtocol，通过函数指针的方式，与FFMPEG连接；
     * 4、剩下的就是调用该URLProtocol的函数进行open,read等操作
     */
    if ((ret = avformat_open_input(&pFormatCtx, filename, NULL, NULL)) < 0) {
        printf("Can't open input file \r\n");
        return ret;
    }
    // 检索媒体流信息
    if ((ret = avformat_find_stream_info(pFormatCtx, NULL)) < 0) {
        printf("Can't find stream information \r\n");
        return ret;
    }
    
    /** select the video stream */
    /** 查找对应的视频流 */
    if ((ret = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0)) < 0) {
        printf("Can't find a video stream in the input file \r\n");
        return ret;
    }
    
    /** 打开对应的编解码器 */
    video_stream_index = ret;
    pCodecCtx = pFormatCtx->streams[video_stream_index]->codec;
    /**
     * pCodecCtx: 需要初始化的AVCodecContext
     * dec: 输入的AVCodec
     */
    if ((ret = avcodec_open2(pCodecCtx, dec, NULL)) < 0) {
        printf("Can't open video decoder \r\n");
        return ret;
    }
    
    return 0;
}

static int init_filters(const char *filters_descr)
{
    char args[512] = {0};
    int ret = -1;
    
    // 获取对应的滤镜对象
    AVFilter *buffersrc = avfilter_get_by_name("buffer");
    AVFilter *buffersink = avfilter_get_by_name("buffersink");
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs = avfilter_inout_alloc();
    enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE };
    AVBufferSinkParams *buffersink_params = NULL;
    
    // 生成 graph
    filter_graph = avfilter_graph_alloc();
    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
             pCodecCtx->time_base.num, pCodecCtx->time_base.den,
             pCodecCtx->sample_aspect_ratio.num, pCodecCtx->sample_aspect_ratio.den);
    // 创建 src
    ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in", args, NULL, filter_graph);
    if (ret < 0) {
        printf("Can't create buffer source \r\n");
        return ret;
    }
    
    /* buffer video sink: to terminate the filter chain. */
    buffersink_params = av_buffersink_params_alloc();
    buffersink_params->pixel_fmts = pix_fmts;
    /* 创建 sink, 可以参考 buffersink 的实现
     * sink_buffer 是一个能通过buffer输出帧的sink
     */
    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out", NULL, buffersink_params, filter_graph);
    av_free(buffersink_params);
    if (ret < 0) {
        printf("Can't create buffer sink \r\n");
        return ret;
    }
    
    /* Endpoints for the filter graph. */
    outputs->name = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx = 0;
    outputs->next = NULL;
    
    inputs->name = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx = 0;
    inputs->next = NULL;
    
    if ((ret = avfilter_graph_parse_ptr(filter_graph, filters_descr, &inputs, &outputs, NULL)) < 0) {
        return ret;
    }
    
    if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)
        return ret;
    
    return 0;
}

int HBFilterWaterTest(int argc, char **argv)
{
    int ret = -1;
    AVPacket packet;
    AVFrame *pFrame = NULL;
    AVFrame *pFrame_out = NULL;
    int got_frame = -1;
    
    av_register_all();
    avfilter_register_all();
    
    if ((ret = open_input_file("/Users/zj-db0519/Desktop/material/folder/video/100f.flv")) < 0)
        goto end;
    if ((ret = init_filters(filter_descr)) < 0)
        goto end;
    
#if ENABLE_YUVFILE
    FILE * fp_yuv = fopen("/Users/zj-db0519/Desktop/material/folder/video/100f2.yuv", "wb+");
#endif
    
#if ENABLE_SDL
    SDL_Rect rect;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        printf("Could't initialize SDL - %s \r\n", SDL_GetError());
        return -1;
    }
    SDL_Surface * screen = SDL_SetVideoMode(pCodecCtx->width, pCodecCtx->height, 0, 0);
    if (!screen) {
        printf("SDL: could not set video mode - exiting \r\n");
        return -1;
    }
    SDL_Overlay * bmp = SDL_CreateYUVOverlay(pCodecCtx->width, pCodecCtx->height,SDL_YV12_OVERLAY, screen);
    SDL_WM_SetCaption("Simplest FFmpeg Video Filter", NULL);
#endif
    
    pFrame = av_frame_alloc();
    pFrame_out = av_frame_alloc();
    
    while (1) {
        if ((ret = av_read_frame(pFormatCtx, &packet)) < 0)
            break;
        
        if (packet.stream_index == video_stream_index) {
            got_frame = 0;
            if ((ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_frame, &packet)) < 0) {
                printf("Error decoding video \r\n");
                break;
            }
            
            if (got_frame) {
                pFrame->pts = av_frame_get_best_effort_timestamp(pFrame);
                
                /* push the decoded frame into the filtergraph */
                if (av_buffersrc_add_frame(buffersrc_ctx, pFrame) < 0) {
                    printf( "Error while feeding the filtergraph \r\n");
                    break;
                }
                
                /* pull filtered pictures from the filtergraph */
                while (1) {
                    if ((ret = av_buffersink_get_frame(buffersink_ctx, pFrame_out)) < 0)
                        break;
                    
                    printf("Process 1 frame! \r\n");
                    if (pFrame_out->format == AV_PIX_FMT_YUV420P) {
#if ENABLE_YUVFILE
                        //Y, U, V
                        for (int i=0; i<pFrame_out->height; i++)
                            fwrite(pFrame_out->data[0]+pFrame_out->linesize[0]*i, 1, pFrame_out->width, fp_yuv);
                        for (int i=0; i<pFrame_out->height/2; i++)
                            fwrite(pFrame_out->data[1]+pFrame_out->linesize[1]*i, 1, pFrame_out->width/2, fp_yuv);
                        for (int i=0; i<pFrame_out->height/2; i++)
                            fwrite(pFrame_out->data[2]+pFrame_out->linesize[2]*i, 1, pFrame_out->width/2, fp_yuv);
#endif
#if ENABLE_SDL
                        SDL_LockYUVOverlay(bmp);
                        int y_size = pFrame_out->width*pFrame_out->height;
                        memcpy(bmp->pixels[0],pFrame_out->data[0],y_size);   //Y
                        memcpy(bmp->pixels[2],pFrame_out->data[1],y_size/4); //U
                        memcpy(bmp->pixels[1],pFrame_out->data[2],y_size/4); //V
                        bmp->pitches[0]=pFrame_out->linesize[0];
                        bmp->pitches[2]=pFrame_out->linesize[1];
                        bmp->pitches[1]=pFrame_out->linesize[2];
                        SDL_UnlockYUVOverlay(bmp);
                        rect.x = 0;
                        rect.y = 0;
                        rect.w = pFrame_out->width;
                        rect.h = pFrame_out->height;
                        SDL_DisplayYUVOverlay(bmp, &rect);
                        //Delay 40ms
                        SDL_Delay(40);
#endif
                    }
                    av_frame_unref(pFrame_out);
                }
            }
            av_frame_unref(pFrame);
        }
        av_free_packet(&packet);
    }
#if ENABLE_YUVFILE
    fclose(fp_yuv);
#endif
    
end:
    avfilter_graph_free(&filter_graph);
    if (pCodecCtx)
        avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
    
    if (ret < 0 && ret != AVERROR_EOF) {
        char buff[1024] = {0};
        av_strerror(ret, buff, sizeof(buff));
        printf("Error occurred: %s\n", buff);
        return -1;
    }
    
    return 0;
}



























