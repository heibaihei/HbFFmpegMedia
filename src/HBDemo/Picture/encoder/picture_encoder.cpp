/**
 * ◊ÓºÚµ•µƒª˘”⁄FFmpegµƒÕºœÒ±‡¬Î∆˜
 * Simplest FFmpeg Picture Encoder
 * 
 * ¿◊œˆÊË Lei Xiaohua
 * leixiaohua1020@126.com
 * ÷–π˙¥´√Ω¥Û—ß/ ˝◊÷µÁ ”ºº ı
 * Communication University of China / Digital TV Technology
 * http://blog.csdn.net/leixiaohua1020
 * 
 * ±æ≥Ã–Ú µœ÷¡ÀYUV420PœÒÀÿ ˝æ›±‡¬ÎŒ™JPEGÕº∆¨°£ «◊ÓºÚµ•µƒFFmpeg±‡¬Î∑Ω√ÊµƒΩÃ≥Ã°£
 * Õ®π˝—ßœ∞±æ¿˝◊”ø…“‘¡ÀΩ‚FFmpegµƒ±‡¬Î¡˜≥Ã°£
 */

#include <stdio.h>

#define __STDC_CONSTANT_MACROS

#ifdef _WIN32
//Windows
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
};
#else
//Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#ifdef __cplusplus
};
#endif
#endif

#include "picture_encoder.h"

/**
 *  @func: HBPictureEncodeYUVtoJPEG
 *  @descript: 程序实现了YUV420P像素数据编码为JPEG图片;
 */
extern int HBPictureEncodeYUVtoJPEG();

int HBPictureEncoder(int argc, char* argv[])
{
    return HBPictureEncodeYUVtoJPEG();
}

int HBPictureEncodeYUVtoJPEG()
{
    AVFormatContext* pFormatCtx = nullptr;
    AVOutputFormat* fmt = nullptr;
    AVStream* video_st = nullptr;
    AVCodecContext* pCodecCtx = nullptr;
    AVCodec* pCodec = nullptr;
    
    uint8_t* picture_buf = nullptr;
    AVPacket pkt;
    
    /** 设置输入文件的相关信息 */
    int inputYUVWidth=480,inputYUVHight=272;
    FILE *inputYUVFile = fopen(RESOURCE_PICTURE_ROOT_PATH"/encoder/cuc_view_480x272.yuv", "rb");
    
    /** 设置输出文件的相关信息 */
    const char* outputFile = RESOURCE_PICTURE_ROOT_PATH"/encoder/output/cuc_view_encode.jpg";    //Output file

    
    av_register_all();
    
    //Method 1
    pFormatCtx = avformat_alloc_context();
    // 查找输出文件的封装格式，指定对应的封装格式
    fmt = av_guess_format("mjpeg", NULL, NULL);
    pFormatCtx->oformat = fmt;
    // 打开输出文件
    if (avio_open(&pFormatCtx->pb, outputFile, AVIO_FLAG_READ_WRITE) < 0){
        printf("Couldn't open output file.");
        return -1;
    }
    
    //Method 2. More simple
    //avformat_alloc_output_context2(&pFormatCtx, NULL, NULL, outputFile);
    //fmt = pFormatCtx->oformat;
    
    // 在AVFormatContext 中创建一条视频流
    video_st = avformat_new_stream(pFormatCtx, 0);
    if (video_st==NULL){
        return -1;
    }
    // 初始化 CodecContext
    pCodecCtx = video_st->codec;
    pCodecCtx->codec_id = fmt->video_codec;
    pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    pCodecCtx->pix_fmt = AV_PIX_FMT_YUVJ420P;
    pCodecCtx->width = inputYUVWidth;
    pCodecCtx->height = inputYUVHight;
    pCodecCtx->time_base.num = 1;
    pCodecCtx->time_base.den = 25;
    
    //Output some information
    av_dump_format(pFormatCtx, 0, outputFile, 1);
    
    // 搜索对应的编解码器
    pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
    if (!pCodec){
        printf("Codec not found.");
        return -1;
    }
    
    // 打开解码器
    if (avcodec_open2(pCodecCtx, pCodec,NULL) < 0){
        printf("Could not open codec.");
        return -1;
    }
    // 分配图像空间
    AVFrame* picture = av_frame_alloc();
    int size = avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
    picture_buf = (uint8_t *)av_malloc(size);
    if (!picture_buf)
    {
        return -1;
    }
    avpicture_fill((AVPicture *)picture, picture_buf, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
    
    // 写入图片格式头部
    avformat_write_header(pFormatCtx,NULL);
    
    int numOfPixel = pCodecCtx->width * pCodecCtx->height;
    av_new_packet(&pkt,numOfPixel*3);
    // 从文件中读取 YUV 数据到缓冲区中
    if (fread(picture_buf, 1, numOfPixel*3/2, inputYUVFile) <=0)
    {
        printf("Could not read input file.");
        return -1;
    }
    picture->data[0] = picture_buf;                  // Y
    picture->data[1] = picture_buf+ numOfPixel;      // U
    picture->data[2] = picture_buf+ numOfPixel*5/4;  // V
    
    // 进行数据编码，将 AVFrame 转化成 AVPacket
    int got_picture=0;
    int ret = avcodec_encode_video2(pCodecCtx, &pkt, picture, &got_picture);
    if(ret < 0){
        printf("Encode Error.\n");
        return -1;
    }
    if (got_picture==1){
        pkt.stream_index = video_st->index;
        // 将 AVPacket 写到媒体文件中
        ret = av_write_frame(pFormatCtx, &pkt);
    }
    
    av_free_packet(&pkt);
    
    // 将媒体文件尾写入媒体文件中
    av_write_trailer(pFormatCtx);
    
    printf("Encode Successful.\n");
    
    if (video_st){
        avcodec_close(video_st->codec);
        av_free(picture);
        av_free(picture_buf);
    }
    
    avio_close(pFormatCtx->pb);
    avformat_free_context(pFormatCtx);
    fclose(inputYUVFile);
    
    return 0;
}
