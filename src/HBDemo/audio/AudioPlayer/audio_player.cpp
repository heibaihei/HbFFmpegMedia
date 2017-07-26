
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define __STDC_CONSTANT_MACROS

//Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <SDL2/SDL.h>
    
#include "audio_player.h"
#ifdef __cplusplus
};
#endif


#define FFMPEG_AUDIO_PLAYER_MATERIAL_PATH "/Users/zj-db0519/work/code/study/ffmpeg/proj/ffmpeg_3_2/xcode/src/HBDemo/simplest_ffmpeg_audio_player/"

#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio，

//Output PCM
#define OUTPUT_PCM 1
//Use SDL
#define USE_SDL 1

//Buffer:
//|-----------|-------------|
//chunk-------pos---len-----|
static  Uint8  *audio_chunk; 
static  Uint32  audio_len; 
static  Uint8  *audio_pos; 

/* The audio function callback takes the following parameters: 
 * stream: A pointer to the audio buffer to be filled 
 *         一个指向音频数据缓冲区的指针，该缓冲区的数据由外部来填充，该缓冲区由音频设备提供；
 * len: The length (in bytes) of the audio buffer 
 *         缓冲区的大小，以字节为单位
*/ 
void  fill_audio(void *udata,Uint8 *stream,int len){ 
	//SDL 2.0
	SDL_memset(stream, 0, len);
    
    // 如果外部无音频数据，则直接返回
	if(audio_len == 0)
		return; 

    // 尽可能传递多的音频数据
	len=((len > audio_len) ? audio_len:len);	/*  Mix  as  much  data  as  possible  */

    // 将从 audio_pos 开始的音频数据填充到 音频设备的缓冲区中
	SDL_MixAudio(stream, audio_pos, len, SDL_MIX_MAXVOLUME);
    
    // 将 audio_pos 往后移动输出的音频数据长度，同时记录外部尚存的音频数据
	audio_pos += len;
	audio_len -= len; 
} 
//-----------------


int simplest_ffmpeg_audio_player(int argc, char* argv[])
{
	AVFormatContext	*pFormatCtx = nullptr;
	int				audioStream;
	AVCodecContext	*pCodecCtx = nullptr;
	AVCodec			*pCodec = nullptr;
	AVPacket		*packet = nullptr;
	uint8_t			*out_buffer = nullptr;
	AVFrame			*pFrame = nullptr;
    
	SDL_AudioSpec wanted_spec;
    int ret;
	uint32_t len = 0;
	int got_picture;
	int index = 0;
	int64_t in_channel_layout;
	

	FILE *pFile = nullptr;
	char url[]=FFMPEG_AUDIO_PLAYER_MATERIAL_PATH"xiaoqingge.mp3";

    
	av_register_all();
	avformat_network_init();
    
    
    // 由外部创建 avformat 空间
	pFormatCtx = avformat_alloc_context();
	if(avformat_open_input(&pFormatCtx, url, NULL, NULL) !=0 ){
		printf("Couldn't open input stream.\n");
		return -1;
	}

	if(avformat_find_stream_info(pFormatCtx, NULL) < 0){
		printf("Couldn't find stream information.\n");
		return -1;
	}
    // 输出检测到的音频数据
	av_dump_format(pFormatCtx, 0, url, false);

    
	audioStream=-1;
    for(int i=0; i < pFormatCtx->nb_streams; i++) {
		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO){
			audioStream=i;
			break;
		}
    }
    
	if(audioStream == -1){
		printf("Didn't find a audio stream.\n");
		return -1;
	}


    // 获取对应的 codec
	pCodecCtx = pFormatCtx->streams[audioStream]->codec;
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if(pCodec == nullptr){
		printf("Codec not found.\n");
		return -1;
	}
    // 打开对应的媒体
	if(avcodec_open2(pCodecCtx, pCodec,NULL)<0){
		printf("Could not open codec.\n");
		return -1;
	}

	
#if OUTPUT_PCM
	pFile=fopen(FFMPEG_AUDIO_PLAYER_MATERIAL_PATH"output.pcm", "wb");
#endif
    // 外部创建 AVPacket
	packet=(AVPacket *)av_malloc(sizeof(AVPacket));
	av_init_packet(packet);

    
	//nb_samples: AAC-1024 MP3-1152
	int out_nb_samples = pCodecCtx->frame_size;
	AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
	int out_sample_rate = 44100;
    /*** Out Audio Param , 立体声，双声道 */
    uint64_t out_channel_layout=AV_CH_LAYOUT_STEREO;
	int out_channels = av_get_channel_layout_nb_channels(out_channel_layout);
    
	// 计算： 读取一次音频采样数据，需要的数据缓冲区大小
	int out_buffer_size=av_samples_get_buffer_size(NULL, out_channels, out_nb_samples, out_sample_fmt, 1);
    out_buffer=(uint8_t *)av_malloc(out_buffer_size);// MAX_AUDIO_FRAME_SIZE*2);
	pFrame=av_frame_alloc();

    
#if USE_SDL
	//Init
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {  
		printf( "Could not initialize SDL - %s\n", SDL_GetError()); 
		return -1;
	}
	//SDL_AudioSpec
	wanted_spec.freq = out_sample_rate; 
	wanted_spec.format = AUDIO_S16SYS; 
	wanted_spec.channels = out_channels; 
	wanted_spec.silence = 0; 
	wanted_spec.samples = out_nb_samples; 
	wanted_spec.callback = fill_audio; 
	wanted_spec.userdata = pCodecCtx; 

	if (SDL_OpenAudio(&wanted_spec, NULL)<0){ 
		printf("can't open audio.\n"); 
		return -1; 
	} 
#endif

	//FIX:Some Codec's Context Information is missing
	in_channel_layout = av_get_default_channel_layout(pCodecCtx->channels);
    
	//Swr 实例化SwrContext，并设置转换所需的参数：通道数量、channel layout、sample rate
	struct SwrContext * audio_convert_ctx = swr_alloc();
	audio_convert_ctx = swr_alloc_set_opts(audio_convert_ctx,out_channel_layout, out_sample_fmt, out_sample_rate, in_channel_layout,pCodecCtx->sample_fmt, pCodecCtx->sample_rate,0, NULL);
	swr_init(audio_convert_ctx);

	//Play
	SDL_PauseAudio(0);

	while(av_read_frame(pFormatCtx, packet)>=0)
    {
		if(packet->stream_index == audioStream)
        {
			ret = avcodec_decode_audio4(pCodecCtx, pFrame, &got_picture, packet);
			if ( ret < 0 ) {
                printf("Error in decoding audio frame.\n");
                return -1;
            }
            
			if ( got_picture > 0 )
            {
				swr_convert(audio_convert_ctx,&out_buffer, MAX_AUDIO_FRAME_SIZE,(const uint8_t **)pFrame->data , pFrame->nb_samples);
#if OUTPUT_PCM
				fwrite(out_buffer, 1, out_buffer_size, pFile);
#endif
                printf("index:%5d\t pts:%lld\t packet size:%d\n",index,packet->pts,packet->size);
				index++;
			}

#if USE_SDL
			while(audio_len > 0)//Wait until finish
				SDL_Delay(1); 

			//Set audio buffer (PCM data)
			audio_chunk = (Uint8 *) out_buffer; 
			//Audio buffer length
			audio_len = out_buffer_size;
			audio_pos = audio_chunk;
#endif
		}
        
		av_free_packet(packet);
	}

	swr_free(&audio_convert_ctx);

#if USE_SDL
	SDL_CloseAudio();//Close SDL
	SDL_Quit();
#endif
	
#if OUTPUT_PCM
	fclose(pFile);
#endif
	av_free(out_buffer);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);

	return 0;
}


