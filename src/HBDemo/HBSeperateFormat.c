#include <stdio.h>

#define __STDC_CONSTANT_MACROS

#include "libavformat/avformat.h"
#include "libavutil/mathematics.h"

#define USE_H264BSF  1

int HBSeperateFormat(int argc, char **argv)
{
    AVFormatContext *ifmt_ctx = NULL;
    AVPacket    pkt;
    int ret, i;
    int videoindex=-1, audioindex = -1;
    const char *in_filename = "/Users/zj-db0519/work/code/study/ffmpeg/proj/ffmpeg_3_2/xcode/product/100.mp4";
    const char *out_filename_v = "/Users/zj-db0519/work/code/study/ffmpeg/proj/ffmpeg_3_2/xcode/product/cuc_ieschool.h264";
    const char *out_filename_a = "/Users/zj-db0519/work/code/study/ffmpeg/proj/ffmpeg_3_2/xcode/product/cuc_ieschool.mp3";
    
    av_register_all();
    if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0) {
        printf( "Could not open input file.");
        return -1;
    }
    
    if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
        printf( "Failed to retrieve input stream information");
        return -1;
    }
    
    videoindex = -1;
    for (i=0; i<ifmt_ctx->nb_streams; i++) {
        if (ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoindex = i;
        } else if (ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioindex = i;
        }
    }
    printf("\nInput Video===========================\n");
    av_dump_format(ifmt_ctx, 0, in_filename, 0);
    printf("\n======================================\n");
    
    FILE *fp_audio = fopen(out_filename_a, "wb+");
    FILE *fp_video = fopen(out_filename_v, "wb+");
    
#if USE_H264BSF
    AVBitStreamFilterContext* h264bsfc = av_bitstream_filter_init("h264_mp4toannexb");
#endif
    
    // 逐个读取包数据
    while (av_read_frame(ifmt_ctx, &pkt) >= 0) {
        if (pkt.stream_index == videoindex) {
#if USE_H264BSF
            // 逐个过滤每次读取到的包数据，添加 SPS 和 PPS 信息
            av_bitstream_filter_filter(h264bsfc, ifmt_ctx->streams[videoindex]->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
#endif
            printf("Write Video Packet. size:%d\tpts:%lld\n",pkt.size,pkt.pts);
            // 将数据写入文件
            fwrite(pkt.data, 1, pkt.size, fp_video);
        } else if (pkt.stream_index == audioindex) {
            /*
                 AAC in some container format (FLV, MP4, MKV etc.) need to add 7 Bytes
                 ADTS Header in front of AVPacket data manually.
                 Other Audio Codec (MP3...) works well.
             */
            printf("Write Audio Packet. size:%d\tpts:%lld\n",pkt.size,pkt.pts);
            fwrite(pkt.data, 1, pkt.size, fp_audio);
        }
        av_free_packet(&pkt);
    }
#if USE_H264BSF
    av_bitstream_filter_close(h264bsfc);
#endif
    
    fclose(fp_video);
    fclose(fp_audio);
    
    avformat_close_input(&ifmt_ctx);
    
    if (ret < 0 && ret != AVERROR_EOF) {
        printf( "Error occurred.\n");
        return -1;
    }
    return 0;
}


/********************************************************************************************
 *********************************************************************************************
 *********************************************************************************************
 *********************************************************************************************
 *********************************************************************************************/

// 视音频分离器
/**
 * 基于FFmpeg的视音频分离器（Simplest FFmpeg demuxer）。视音频分离器（Demuxer）即是将封装格式数据（例如MKV）中的视频压缩数据（例如H.264）和音频压缩数据（例如AAC）分离开。如图所示。在这个过程中并不涉及到编码和解码。
 
    将一个MPEG2TS封装的视频文件（其中视频编码为H.264，音频编码为AAC）分离成为两个文件：一个H.264编码的视频码流文件，一个AAC编码的音频码流文件。
 */


int HBSeperateFormat2(int argc, char **argv)
{
    AVOutputFormat *ofmt_a = NULL, *ofmt_v = NULL;
    AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx_a = NULL, *ofmt_ctx_v = NULL;
    AVPacket pkt;
    int ret , i;
    int videoindex = -1, audioindex = -1;
    int frame_index = 0;
    
    const char *in_filename = "cuc_ieschool.ts";//Input file URL
    //char *in_filename  = "cuc_ieschool.mkv";
    const char *out_filename_v = "cuc_ieschool.h264";//Output file URL
    //char *out_filename_a = "cuc_ieschool.mp3";
    const char *out_filename_a = "cuc_ieschool.aac";
    
    av_register_all();
    //input
    if((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0) {
        printf( "Could not open input file.");
        goto end;
    }
    
    if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
        printf( "Failed to retrieve input stream information");
        goto end;
    }
    
    // output
    avformat_alloc_output_context2(&ofmt_ctx_v, NULL, NULL, out_filename_v);
    if(!ofmt_ctx_v){
        printf( "Could not create output context\n");
        ret = AVERROR_UNKNOWN;
        goto end;
    }
    ofmt_v = ofmt_ctx_v->oformat;
    
    
    avformat_alloc_output_context2(&ofmt_ctx_a, NULL, NULL, out_filename_a);
    if (!ofmt_ctx_a) {
        printf( "Could not create output context\n");
        ret = AVERROR_UNKNOWN;
        goto end;
    }
    ofmt_a = ofmt_ctx_a->oformat;
    
    
    for (i = 0; i<ifmt_ctx->nb_streams; i++) {
        AVFormatContext *ofmt_ctx;
        AVStream *in_stream = ifmt_ctx->streams[i];
        AVStream *out_stream = NULL;
    
        if (ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoindex = i;
            out_stream = avformat_new_stream(ofmt_ctx_v, in_stream->codec->codec);
            ofmt_ctx = ofmt_ctx_v;
        } else if (ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioindex = i;
            out_stream = avformat_new_stream(ofmt_ctx_a, in_stream->codec->codec);
            ofmt_ctx = ofmt_ctx_a;
        }else {
            break;
        }
        
        if (!out_stream) {
            printf( "Failed allocating output stream\n");
            ret = AVERROR_UNKNOWN;
            goto end;
        }
        
        //Copy the settings of AVCodecContext
        if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {
            printf( "Failed to copy context from input to output stream codec context\n");
            goto end;
        }
        
        out_stream->codec->codec_tag = 0;
        if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
            out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }
    
    //Dump Format------------------
    printf("\n==============Input Video=============\n");
    av_dump_format(ifmt_ctx, 0, in_filename, 0);
    printf("\n==============Output Video============\n");
    av_dump_format(ofmt_ctx_v, 0, out_filename_v, 1);
    printf("\n==============Output Audio============\n");
    av_dump_format(ofmt_ctx_a, 0, out_filename_a, 1);
    printf("\n======================================\n");
    
    //Open output file
    if (!(ofmt_v->flags & AVFMT_NOFILE)) {
        if (avio_open(&ofmt_ctx_v->pb, out_filename_v, AVIO_FLAG_WRITE) < 0) {
            printf( "Could not open output file '%s'", out_filename_v);
            goto end;
        }
    }
    
    if (!(ofmt_a->flags & AVFMT_NOFILE)) {
        if (avio_open(&ofmt_ctx_a->pb, out_filename_a, AVIO_FLAG_WRITE) < 0) {
            printf( "Could not open output file '%s'", out_filename_a);
            goto end;
        }
    }
    
    //Write file header
    if (avformat_write_header(ofmt_ctx_v, NULL) < 0) {
        printf( "Error occurred when opening video output file\n");
        goto end;
    }
    
    if (avformat_write_header(ofmt_ctx_a, NULL) < 0) {
        printf( "Error occurred when opening audio output file\n");
        goto end;
    }
    
#if USE_H264BSF
    AVBitStreamFilterContext* h264bsfc = av_bitstream_filter_init("h264_mp4toannexb");
#endif
    
    while (1) {
        AVFormatContext* ofmt_ctx;
        AVStream *in_stream, *out_stream;
        //Get an AVPacket
        if(av_read_frame(ifmt_ctx, &pkt) < 0)
            break;
        in_stream = ifmt_ctx->streams[pkt.stream_index];
        
        if (pkt.stream_index == videoindex) {
            out_stream = ofmt_ctx_v->streams[0];
            ofmt_ctx = ofmt_ctx_v;
            printf("Write Video Packet. size:%d\tpts:%lld\n",pkt.size,pkt.pts);
#if USE_H264BSF
            av_bitstream_filter_filter(h264bsfc, in_stream->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
#endif
        }else if (pkt.stream_index == audioindex) {
            out_stream = ofmt_ctx_a->streams[0];
            ofmt_ctx = ofmt_ctx_a;
            printf("Write Audio Packet. size:%d\tpts:%lld\n",pkt.size,pkt.pts);
        }else {
            continue;
        }
        //Convert PTS/DTS
        /** (int64_t a, int64_t b, int64_t c, enum AVRounding rnd):   计算 "a * b / c" 的值并分五种方式来取整. 
         *  将以 "时钟基b" 表示的 数值a 转换成以 "时钟基c" 来表示
         */
        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (enum AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (enum AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.pos = -1;
        pkt.stream_index=0;
        //Write
        if (av_interleaved_write_frame(ofmt_ctx, &pkt) < 0) {
            printf( "Error muxing packet\n");
            break;
        }
        //printf("Write %8d frames to output file\n",frame_index);
        av_free_packet(&pkt);
        frame_index++;
    }
    
#if USE_H264BSF
    av_bitstream_filter_close(h264bsfc);
#endif
    
    //Write file trailer
    av_write_trailer(ofmt_ctx_a);
    av_write_trailer(ofmt_ctx_v);
end:
    avformat_close_input(&ifmt_ctx);
    /* close output */
    if (ofmt_ctx_a && !(ofmt_a->flags & AVFMT_NOFILE))
        avio_close(ofmt_ctx_a->pb);
    
    if (ofmt_ctx_v && !(ofmt_v->flags & AVFMT_NOFILE))
        avio_close(ofmt_ctx_v->pb);
    
    avformat_free_context(ofmt_ctx_a);
    avformat_free_context(ofmt_ctx_v);
    
    
    if (ret < 0 && ret != AVERROR_EOF) {
        printf( "Error occurred.\n");
        return -1;
    }
    return 0;
}













