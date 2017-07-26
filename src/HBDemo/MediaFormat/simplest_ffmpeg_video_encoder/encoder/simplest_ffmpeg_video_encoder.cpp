/**
 * ◊ÓºÚµ•µƒª˘”⁄FFmpegµƒ ”∆µ±‡¬Î∆˜
 * Simplest FFmpeg Video Encoder
 * 
 * ¿◊œˆÊË Lei Xiaohua
 * leixiaohua1020@126.com
 * ÷–π˙¥´√Ω¥Û—ß/ ˝◊÷µÁ ”ºº ı
 * Communication University of China / Digital TV Technology
 * http://blog.csdn.net/leixiaohua1020
 * 
 * 本程序实现了YUV像素数据编码为视频码流（H264，MPEG2，VP8等等）。
 */

/**
 * 参考链接： http://blog.csdn.net/leixiaohua1020/article/details/25430425
 */

#include <stdio.h>

#define __STDC_CONSTANT_MACROS

#ifdef _WIN32
//Windows
extern "C"
{
#include "libavutil/opt.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
};
#else
//Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#ifdef __cplusplus
};
#endif
#endif

#define SIMPLEST_FFMPEG_VIDEO_ENCODER  "/Users/zj-db0519/work/code/study/ffmpeg/proj/ffmpeg_3_2/xcode/resource/simplest_ffmpeg_video_encoder/"

int video_flush_encoder(AVFormatContext *fmt_ctx,unsigned int stream_index){
	int ret;
	int got_frame;
	AVPacket enc_pkt;
	if (!(fmt_ctx->streams[stream_index]->codec->codec->capabilities & CODEC_CAP_DELAY))
		return 0;
	while (1) {
		enc_pkt.data = NULL;
		enc_pkt.size = 0;
		av_init_packet(&enc_pkt);
		ret = avcodec_encode_video2 (fmt_ctx->streams[stream_index]->codec, &enc_pkt, NULL, &got_frame);
		av_frame_free(NULL);
		if (ret < 0)
			break;
		if (!got_frame){
			ret=0;
			break;
		}
		printf("Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n",enc_pkt.size);
		/* mux encoded frame */
		ret = av_write_frame(fmt_ctx, &enc_pkt);
		if (ret < 0)
			break;
	}
	return ret;
}

int simplest_video_encoder(int argc, char* argv[])
{
	AVFormatContext* pFormatCtx = NULL;
	AVOutputFormat* fmt = NULL;
	AVStream* video_stream = NULL;
	AVCodecContext* pCodecCtx = NULL;
	AVCodec* pCodec = NULL;
	AVPacket pkt;
	uint8_t* picture_buf = NULL;
	AVFrame* pFrame = NULL;
	int picture_size;
	int yuv_size;
	int framecnt=0;
	
    
    /** 输出路径 */
    const char* out_file = SIMPLEST_FFMPEG_VIDEO_ENCODER"ds.h264";
    /** Open a YUV file, to get the raw YUV data */
	FILE *input_YUV_file = fopen(SIMPLEST_FFMPEG_VIDEO_ENCODER"ds_480x272.yuv", "rb");
    /** Input data's width and height */
	int inputWidth=480, inputHeight=272;
    /** The totoal num of frame remain to encode */
	int framenum=100;
    
    
    /** 注册FFmpeg所有编解码器 */
	av_register_all();
    
    
    /** =========================================================== */
    /** 参考链接：http://blog.csdn.net/leixiaohua1020/article/details/41198929 */
	/** Method1.*/
	pFormatCtx = avformat_alloc_context();
	/** 猜测文件格式 */
	if ((fmt = av_guess_format(NULL, out_file, NULL)) == NULL)
        printf("Can't guess the [%s] format !\r\n", out_file);
	pFormatCtx->oformat = fmt;
	
	/** Method2.*/
//	avformat_alloc_output_context2(&pFormatCtx, NULL, NULL, out_file);
//	fmt = pFormatCtx->oformat;
    /** =========================================================== */

	/** 打开输出文件 */
	if (avio_open(&pFormatCtx->pb, out_file, AVIO_FLAG_READ_WRITE) < 0){
		printf("Failed to open output file! \n");
		return -1;
	}

    /** 创建输出码流的AVStream */
	video_stream = avformat_new_stream(pFormatCtx, 0);
	if (video_stream==NULL){
		return -1;
	}
	
    /** 以下为必须配置的参数 */
	pCodecCtx = video_stream->codec;
//	pCodecCtx->codec_id =AV_CODEC_ID_HEVC;
	pCodecCtx->codec_id = fmt->video_codec;
	pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
	pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
	pCodecCtx->width = inputWidth;
	pCodecCtx->height = inputHeight;
	pCodecCtx->bit_rate = 400000;  
	pCodecCtx->gop_size=250;

    /** 设置成期望的 timebase 值 */
	pCodecCtx->time_base.num = 1;  
	pCodecCtx->time_base.den = 25;  

	//H264
	//pCodecCtx->me_range = 16;
	//pCodecCtx->max_qdiff = 4;
	//pCodecCtx->qcompress = 0.6;
	pCodecCtx->qmin = 10;
	pCodecCtx->qmax = 51;

	//Optional Param
	pCodecCtx->max_b_frames=3;

	// Set Option
	AVDictionary *param = 0;
	//H.264
	if(pCodecCtx->codec_id == AV_CODEC_ID_H264) {
		av_dict_set(&param, "preset", "slow", 0);
		av_dict_set(&param, "tune", "zerolatency", 0);
		//av_dict_set(&param, "profile", "main", 0);
	}
	//H.265
	if(pCodecCtx->codec_id == AV_CODEC_ID_H265){
		av_dict_set(&param, "preset", "ultrafast", 0);
		av_dict_set(&param, "tune", "zero-latency", 0);
	}

    
	/** 输出一些媒体信息 */
	av_dump_format(pFormatCtx, 0, out_file, 1);

    
    /** 查找编码器 */
	pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
	if (!pCodec){
		printf("Can not find encoder! \n");
		return -1;
	}
    /** 打开编码器 */
	if (avcodec_open2(pCodecCtx, pCodec,&param) < 0){
		printf("Failed to open encoder! \n");
		return -1;
	}

    /** 创建帧空间 */
	pFrame = av_frame_alloc();
    /** 计算输出媒体格式对应的帧数据需要的数据空间 */
	picture_size = avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
	picture_buf = (uint8_t *)av_malloc(picture_size);
    /** 将数据空间同帧中对应的指针进行绑定 */
	avpicture_fill((AVPicture *)pFrame, picture_buf, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);

	/** 当封装器设置完成后，开始写文件头（对于某些没有文件头的封装格式，不需要此函数。比如说MPEG2TS）
     *  初始化封装器内部结构， 这个阶段是否往IO context里写入东西取决于封装器
     */
	avformat_write_header(pFormatCtx, NULL);

    /** 
     * 参考链接： http://blog.csdn.net/vintionnee/article/details/17734223
     */
	av_new_packet(&pkt, picture_size);

	yuv_size = pCodecCtx->width * pCodecCtx->height;

	for (int i=0; i<framenum; i++){
		/** 从文件中读取 YUV 数据到缓冲区中 */
		if (fread(picture_buf, 1, yuv_size*3/2, input_YUV_file) <= 0){
			printf("Failed to read raw data! \n");
			return -1;
		}else if(feof(input_YUV_file)){
			break;
		}
        /**  */
		pFrame->data[0] = picture_buf;                // Y
		pFrame->data[1] = picture_buf+ yuv_size;      // U
		pFrame->data[2] = picture_buf+ yuv_size*5/4;  // V

        /** 【似乎非必需】设置 pFrame 的相关媒体参数，媒体格式、媒体像素的框高 */
        pFrame->width = 480;
        pFrame->height = 272;
        pFrame->format = AV_PIX_FMT_YUV420P;
        
		/** 计算每一帧对应的 pts 时间 
         * i *（ (1/帧率) ／ 25 ）  ==  i * ( den / (num * 25) )
         */
		pFrame->pts=i*(video_stream->time_base.den)/((video_stream->time_base.num)*25);
        
		int got_picture=0;
        /** 将 pFrame 中的 raw 数据编码写入到 pkt 对应的包中 */
		int ret = avcodec_encode_video2(pCodecCtx, &pkt, pFrame, &got_picture);
		if(ret < 0){
			printf("Failed to encode! \n");
			return -1;
		}
		if (got_picture==1){
			printf("Succeed to encode frame: %5d\tsize:%5d\n",framecnt,pkt.size);
			framecnt++;
			pkt.stream_index = video_stream->index;
            
            /** 发送到封装器的packet的时间信息必须对应 AVStream 的时间基数,
             *  这个时间基数会在 avformat_write_header() 步骤写入,有可能与调用者设置的不同
             */
			ret = av_write_frame(pFormatCtx, &pkt);
            /**  释放packet，包括其data引用的数据缓存 */
			av_free_packet(&pkt);
		}
	}
	//Flush Encoder
	int ret = video_flush_encoder(pFormatCtx,0);
	if (ret < 0) {
		printf("Flushing encoder failed\n");
		return -1;
	}

	/** 当所有数据都写入后, 调用者必须调用 av_write_trailer() 进行文件缓冲数据的关闭和文件的关闭.  */
	av_write_trailer(pFormatCtx);

	//Clean
	if (video_stream){
		avcodec_close(video_stream->codec);
		av_free(pFrame);
		av_free(picture_buf);
	}
    /** 关闭IO context */
	avio_close(pFormatCtx->pb);
    /** 释放封装context */
	avformat_free_context(pFormatCtx);

	fclose(input_YUV_file);

	return 0;
}

