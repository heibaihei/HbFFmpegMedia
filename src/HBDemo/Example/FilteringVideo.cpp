/*
 * Copyright (c) 2010 Nicolas George
 * Copyright (c) 2011 Stefano Sabatini
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * @file
 * API example for decoding and filtering
 * @example filtering_video.c
 */

#define _XOPEN_SOURCE 600 /* for usleep */
#include "HBExample.h"
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfiltergraph.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>

#ifdef __cplusplus
};
#endif

#include "CSLog.h"

/**
 *  采用的滤镜类型以及相关参数
 */
const char *gFilter_descr = "scale=78:24,transpose=cclock";
/* other way:
   scale=78:24 [scl]; [scl] transpose=cclock // assumes "[in]" and "[out]" to be input output pads respectively
 */

static AVFormatContext *gFmt_ctx;
static AVCodecContext *gDec_ctx;
AVFilterContext *gBuffersink_ctx;
AVFilterContext *gBuffersrc_ctx;
/**
 *  AVFilterGraph: 管理filter的，可以看成filter的一个容器
 */
AVFilterGraph *gFilter_graph;
static int video_stream_index = -1;
static int64_t last_pts = AV_NOPTS_VALUE;

static int open_input_file(const char *filename)
{
    int ret;
    AVCodec *dec=NULL;

    if ((ret = avformat_open_input(&gFmt_ctx, filename, NULL, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
        return ret;
    }

    if ((ret = avformat_find_stream_info(gFmt_ctx, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
        return ret;
    }

    /**
     *  select the video stream 
     *  @return: 返回对应流的索引
     */
    ret = av_find_best_stream(gFmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find a video stream in the input file\n");
        return ret;
    }
    video_stream_index = ret;
    gDec_ctx = gFmt_ctx->streams[video_stream_index]->codec;
    /** 帧引用计数 */
    av_opt_set_int(gDec_ctx, "refcounted_frames", 1, 0);

    /* init the video decoder */
    if ((ret = avcodec_open2(gDec_ctx, dec, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open video decoder\n");
        return ret;
    }

    return 0;
}

/**
 *  初始化滤镜链
 */
static int init_filters(const char *filters_descr)
{
    char args[512];
    int ret = 0;
    AVFilter *buffersrc  = avfilter_get_by_name("buffer");
    AVFilter *buffersink = avfilter_get_by_name("buffersink");
    
    /**
     *  纯粹创建滤镜的对象空间
     */
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs  = avfilter_inout_alloc();
    
    AVRational time_base = gFmt_ctx->streams[video_stream_index]->time_base;
    enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_GRAY8, AV_PIX_FMT_NONE };
    
    /** 为FilterGraph分配内存 */
    gFilter_graph = avfilter_graph_alloc();
    if (!outputs || !inputs || !gFilter_graph) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    /* buffer video source: the decoded frames from the decoder will be inserted here. */
    snprintf(args, sizeof(args),
            "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
            gDec_ctx->width, gDec_ctx->height, gDec_ctx->pix_fmt,
            time_base.num, time_base.den,
            gDec_ctx->sample_aspect_ratio.num, gDec_ctx->sample_aspect_ratio.den);
    
    /** 创建并向FilterGraph中添加一个Filter */
    ret = avfilter_graph_create_filter(&gBuffersrc_ctx, buffersrc, "in",
                                       args, NULL, gFilter_graph);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source\n");
        goto end;
    }

    /* buffer video sink: to terminate the filter chain. */
    ret = avfilter_graph_create_filter(&gBuffersink_ctx, buffersink, "out",
                                       NULL, NULL, gFilter_graph);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink\n");
        goto end;
    }

    ret = av_opt_set_int_list(gBuffersink_ctx, "pix_fmts", pix_fmts,
                              AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot set output pixel format\n");
        goto end;
    }

    /*
     * Set the endpoints for the filter graph. The gFilter_graph will
     * be linked to the graph described by filters_descr.
     */

    /*
     * The buffer source output must be connected to the input pad of
     * the first filter described by filters_descr; since the first
     * filter input label is not specified, it is set to "in" by
     * default.
     */
    outputs->name       = av_strdup("in");
    outputs->filter_ctx = gBuffersrc_ctx;
    outputs->pad_idx    = 0;
    outputs->next       = NULL;

    /*
     * The buffer sink input must be connected to the output pad of
     * the last filter described by filters_descr; since the last
     * filter output label is not specified, it is set to "out" by
     * default.
     */
    inputs->name       = av_strdup("out");
    inputs->filter_ctx = gBuffersink_ctx;
    inputs->pad_idx    = 0;
    inputs->next       = NULL;

    /** 将一串通过字符串描述的Graph添加到FilterGraph中 */
    if ((ret = avfilter_graph_parse_ptr(gFilter_graph, filters_descr,
                                    &inputs, &outputs, NULL)) < 0)
        goto end;

    /** 检查FilterGraph的配置 */
    if ((ret = avfilter_graph_config(gFilter_graph, NULL)) < 0)
        goto end;

end:
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);

    return ret;
}

static void display_frame(const AVFrame *frame, AVRational time_base)
{
    int x, y;
    uint8_t *p0, *p;
    int64_t delay;

    if (frame->pts != AV_NOPTS_VALUE) {
        if (last_pts != AV_NOPTS_VALUE) {
            /* sleep roughly the right amount of time;
             * usleep is in microseconds, just like AV_TIME_BASE. */
            delay = av_rescale_q(frame->pts - last_pts,
                                 time_base, AV_TIME_BASE_Q);
            if (delay > 0 && delay < 1000000)
                usleep(delay);
        }
        last_pts = frame->pts;
    }

    /* Trivial ASCII grayscale display. */
    p0 = frame->data[0];
    puts("\033c");
    for (y = 0; y < frame->height; y++) {
        p = p0;
        for (x = 0; x < frame->width; x++)
            putchar(" .-+#"[*(p++) / 52]);
        putchar('\n');
        p0 += frame->linesize[0];
    }
    fflush(stdout);
}

int examples_filtering_video(int argc, char **argv)
{
    int ret;
    AVPacket packet;
    /** 单纯创建 AVFrame 空间资源，并执行相应的初始化操作 */
    AVFrame *frame = av_frame_alloc();
    AVFrame *filter_frame = av_frame_alloc();
    int got_frame;

    if (!frame || !filter_frame) {
        perror("Could not allocate frame");
        exit(1);
    }
    if (argc != 2) {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        exit(1);
    }
    
    av_register_all();
    avfilter_register_all();

    /** 
     *  打开媒体文件 
     */
    if ((ret = open_input_file(argv[1])) < 0)
        goto end;
    
    /**
     *  初始化滤镜链
     */
    if ((ret = init_filters(gFilter_descr)) < 0)
        goto end;

    while (1) { /** 读取数据包 */
        if ((ret = av_read_frame(gFmt_ctx, &packet)) < 0)
            break;

        if (packet.stream_index == video_stream_index)
        {
            got_frame = 0;
            ret = avcodec_decode_video2(gDec_ctx, frame, &got_frame, &packet);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Error decoding video\n");
                break;
            }

            if (got_frame)
            {
                frame->pts = av_frame_get_best_effort_timestamp(frame);

                /**
                 *  push the decoded frame into the filtergraph;
                 *  向FilterGraph中加入一个AVFrame
                 */
                if (av_buffersrc_add_frame_flags(gBuffersrc_ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
                    av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
                    break;
                }

                /* pull filtered frames from the filtergraph */
                while (1) {
                    /** 从FilterGraph中取出一个AVFrame */
                    ret = av_buffersink_get_frame(gBuffersink_ctx, filter_frame);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                        LOGE("[line:%d] av_buffersink_get_frame : %s", __LINE__, av_err2str(ret));
                        break;
                    }
                    if (ret < 0) {
                        LOGE("[line:%d] av_buffersink_get_frame : %s", __LINE__, av_err2str(ret));
                        goto end;
                    }
                    
                    ;
                    LOGD("[line:%d] av_buffersink_get_frame : %lld <stream:%d-%d> <filter:%d-%d>", __LINE__, filter_frame->pts, \
                         gFmt_ctx->streams[video_stream_index]->time_base.den, gFmt_ctx->streams[video_stream_index]->time_base.num, \
                         gBuffersink_ctx->inputs[0]->time_base.den, gBuffersink_ctx->inputs[0]->time_base.num);
                    display_frame(filter_frame, gBuffersink_ctx->inputs[0]->time_base);
                    av_frame_unref(filter_frame);
                }
                av_frame_unref(frame);
            }
        }
        av_packet_unref(&packet);
    }
end:
    // 释放
    avfilter_graph_free(&gFilter_graph);
    avcodec_close(gDec_ctx);
    avformat_close_input(&gFmt_ctx);
    av_frame_free(&frame);
    av_frame_free(&filter_frame);

    if (ret < 0 && ret != AVERROR_EOF) {
        fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
        exit(1);
    }

    exit(0);
}


