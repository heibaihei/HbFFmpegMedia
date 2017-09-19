/*
 * Copyright (c) 2014 Stefano Sabatini
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
 * libavformat AVIOContext API example.
 *
 * Make libavformat demuxer access media content through a custom
 * AVIOContext read callback.
 * @example avio_reading.c
 */
#include "HBExample.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/file.h>

#ifdef __cplusplus
};
#endif

struct buffer_handle {
    uint8_t *ptr;
    size_t size; ///< size left in the buffer
};

/**
 * @func: read_packet
 * @descript: 将 opaque 中数据读取到 buf 指向的空间中，将拷贝的数据大小存储在 buf_size 变量中输出
 */
static int read_packet(void *opaque, uint8_t *buf, int buf_size)
{
    struct buffer_handle *pBufferHandle = (struct buffer_handle *)opaque;
    
    /** 计算最小妖读取的数据大小 */
    buf_size = FFMIN(buf_size, ((int)(pBufferHandle->size)));

    printf("ptr:%p size:%zu\n", pBufferHandle->ptr, pBufferHandle->size);

    /* copy internal buffer data to buf */
    memcpy(buf, pBufferHandle->ptr, buf_size);
    
    /** 更新 opaque 中缓存的数据 */
    pBufferHandle->ptr  += buf_size;
    pBufferHandle->size -= buf_size;

    return buf_size;
}

#define MAX_AVIO_CTX_BUFFER_SIZE (4096)
int demo_avio_reading(int argc, const char *argv[])
{
    AVFormatContext *fmt_ctx = NULL;
    AVIOContext *avio_ctx = NULL;
    uint8_t *avio_ctx_buffer = NULL;
    size_t avio_ctx_buffer_size = MAX_AVIO_CTX_BUFFER_SIZE;
    struct buffer_handle stBufferHandle = { NULL, 0 };

    if (argc != 2) {
        fprintf(stderr, "usage: %s input_file\n"
                "API example program to show how to read from a custom buffer "
                "accessed through AVIOContext.\n", argv[0]);
        return 1;
    }
    char *input_filename = (char *)(argv[1]);

    /* register codecs and formats and other lavf/lavc components*/
    av_register_all();

    /* slurp file content into buffer */
    int ret = av_file_map(input_filename, &(stBufferHandle.ptr), &(stBufferHandle.size), 0, NULL);
    if (ret < 0)
        goto end;

    if (!(fmt_ctx = avformat_alloc_context())) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    avio_ctx_buffer = (uint8_t *)av_malloc(avio_ctx_buffer_size);
    if (!avio_ctx_buffer) {
        ret = AVERROR(ENOMEM);
        goto end;
    }
    avio_ctx = avio_alloc_context(avio_ctx_buffer, (int)avio_ctx_buffer_size,
                                  0, &stBufferHandle, &read_packet, NULL, NULL);
    if (!avio_ctx) {
        ret = AVERROR(ENOMEM);
        goto end;
    }
    fmt_ctx->pb = avio_ctx;

    ret = avformat_open_input(&fmt_ctx, NULL, NULL, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not open input\n");
        goto end;
    }

    ret = avformat_find_stream_info(fmt_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not find stream information\n");
        goto end;
    }

    av_dump_format(fmt_ctx, 0, input_filename, 0);

end:
    avformat_close_input(&fmt_ctx);
    /* note: the internal buffer could have changed, and be != avio_ctx_buffer */
    if (avio_ctx) {
        av_freep(&avio_ctx->buffer);
        av_freep(&avio_ctx);
    }
    av_file_unmap(stBufferHandle.ptr, stBufferHandle.size);

    if (ret < 0) {
        fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
        return 1;
    }

    return 0;
}
