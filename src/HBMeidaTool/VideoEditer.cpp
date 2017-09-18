#include "VideoEditer.h"
#include "FrameRecorder.h"
#include "scale_yuv.h"
#include "rotate_argb.h"
#include "LogHelper.h"

#include <vector>
#include <cstdint>
#ifndef LOGE
#define LOGE printf
#endif

#ifndef makeErrorStr
static char errorStr[AV_ERROR_MAX_STRING_SIZE];
#define makeErrorStr(errorCode) av_make_error_string(errorStr, AV_ERROR_MAX_STRING_SIZE, errorCode)
#endif

#ifndef SAFE_DELETE
#define SAFE_DELETE(p) if(p){delete (p); (p) = NULL;}
#endif

#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) if(p){delete [] (p); (p) = NULL;}
#endif

#ifndef INT64_MIN
#define INT64_MIN        (-0x7fffffffffffffff - 1)
#endif

#ifndef INT64_MAX
#define INT64_MAX	0x7fffffffffffffff
#endif

/*
 首先，将一幅1920*1080的风景图像，缩放为400*300的24位RGB，下面的帧率，是指每秒钟缩放并渲染的次数。（经过我的测试，渲染的时间可以忽略不计，主要时间还是耗费在缩放算法上。）
 
 算法                 帧率          图像主观感受
 SWS_FAST_BILINEAR      228         图像无明显失真，感觉效果很不错。
 SWS_BILINEAR           95          感觉也很不错，比上一个算法边缘平滑一些。
 SWS_BICUBIC            80          感觉差不多，比上上算法边缘要平滑，比上一算法要锐利。
 SWS_X                  91          与上一图像，我看不出区别。
 SWS_POINT              427         细节比较锐利，图像效果比上图略差一点点。
 SWS_AREA               116         与上上算法，我看不出区别。
 SWS_BICUBLIN           87          同上。
 SWS_GAUSS              80          相对于上一算法，要平滑(也可以说是模糊)一些。
 SWS_SINC               30          相对于上一算法，细节要清晰一些。
 SWS_LANCZOS            70          相对于上一算法，要平滑(也可以说是模糊)一点点，几乎无区别。
 SWS_SPLINE             47          和上一个算法，我看不出区别。
 
 第二个试验，将一幅1024*768的风景图像，放大到1920*1080，并进行渲染（此时的渲染时间，虽然不是忽略不计，但不超过5ms的渲染时间，不影响下面结论的相对准确性）。
 
 算法                     帧率          图像主观感受
 SWS_FAST_BILINEAR      103         图像无明显失真，感觉效果很不错。
 SWS_BILINEAR           100         和上图看不出区别。
 SWS_BICUBIC             78         相对上图，感觉细节清晰一点点。
 SWS_X                  106         与上上图无区别。
 SWS_POINT              112         边缘有明显锯齿。
 SWS_AREA               114         边缘有不明显锯齿。
 SWS_BICUBLIN           95          与上上上图几乎无区别。
 SWS_GAUSS              86          比上图边缘略微清楚一点。
 SWS_SINC               20          与上上图无区别。
 SWS_LANCZOS            64          与上图无区别。
 SWS_SPLINE             40          与上图无区别。
 */

#define SWS_FLAGS SWS_AREA

//#define MT_MEIPAI_PREVIEW_SIZE 480
// 定义竖屏的宽度和高度
static int MT_IMPORT_SIZE_W = 360;
static int MT_IMPORT_SIZE_H = 640;

static __inline int RGBToY(uint8 r, uint8 g, uint8 b) {
	return (66 * r + 129 * g +  25 * b + 0x1080) >> 8;
}

static __inline int RGBToU(uint8 r, uint8 g, uint8 b) {
	return (112 * b - 74 * g - 38 * r + 0x8080) >> 8;
}
static __inline int RGBToV(uint8 r, uint8 g, uint8 b) {
	return (112 * r - 94 * g - 18 * b + 0x8080) >> 8;
}


static int open_codec_context(int *stream_idx, AVFormatContext *fmt_ctx, AVCodecContext **dec_ctx, enum AVMediaType type)
{
	int ret;
	AVStream *st;
	AVCodec *dec = NULL;
	*stream_idx = -1;
	if ((ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0)) < 0)
	{
		LOGE("Could not find %s stream !(%s)\n",
			av_get_media_type_string(type), makeErrorStr(ret));
		return ret;
	}
	else
	{
		*stream_idx = ret;
		st = fmt_ctx->streams[*stream_idx];
		/* find decoder for the stream */
		*dec_ctx = st->codec;
		dec = avcodec_find_decoder((*dec_ctx)->codec_id);
		if (!dec)
		{
			LOGE("Failed to find %s codec(%s) codec id:%d\n",
				av_get_media_type_string(type), makeErrorStr(ret),(*dec_ctx)->codec_id);
			*stream_idx = -1;
			return -1;
		}
		if ((ret = avcodec_open2(*dec_ctx, dec, NULL)) < 0)
		{
			LOGE("Failed to open %s codec(%s)\n",
				av_get_media_type_string(type), makeErrorStr(ret));
			return ret;
		}
	}
	return 0;
}


static void *aligned_malloc(size_t required_bytes, size_t alignment) {
	void *p1;
	void **p2;
	size_t offset = alignment - 1 + sizeof(void*);
	p1 = malloc(required_bytes + offset);               // the line you are missing
	p2 = (void**)(((size_t)(p1)+offset)&~(alignment - 1));  //line 5
	p2[-1] = p1; //line 6
	return p2;
}

static void aligned_free(void* p) {
	void* p1 = ((void**)p)[-1];         // get the pointer to the buffer we allocated
	free(p1);
}


static void SaveFrameARGB(unsigned char* pARGB, int width, int height, const char* szFilename,double pts)
{
	LOGE("Save FrameARGB file=%s",szFilename);
	FILE *pFile = NULL;

	// Open file
	pFile = fopen(szFilename, "wb");
	if (pFile == NULL)
		return;
	// Write header
/*	fprintf(pFile, "P6\n%d %d\n255\n", width, height);*/
	fwrite((void*)(&width),sizeof(int),1,pFile);
	fwrite((void*)(&height),sizeof(int),1,pFile);
	fwrite((void*)(&pts),sizeof(double),1,pFile);
	// Write pixel data
	fwrite(pARGB, 1, width*height*4, pFile);
	/*for (int i = width*height; i > 0; i--)
	{
		fwrite(pARGB, 1, 3, pFile);
		pARGB += 4;
	}*/
	// Close file
	fclose(pFile);
}


EditorEntity* EditorEntityCreate(AVFrame* frame,double time_stemp)
{
	if (frame->format != AV_PIX_FMT_YUV420P)
	{
		LOGE("pixel format != AV_PIX_FMT_YUV420P");
		return NULL;
	}
	EditorEntity* pEntity = new EditorEntity();
	pEntity->width = frame->width;
	pEntity->height = frame->height;
	pEntity->pts = time_stemp;
	pEntity->pY = new unsigned char[frame->linesize[0] * pEntity->height];
	pEntity->stride_y = frame->linesize[0];
	memcpy(pEntity->pY, frame->data[0], frame->linesize[0] * pEntity->height);
	pEntity->pU = new unsigned char[frame->linesize[1] * pEntity->height/2];
	pEntity->stride_u = frame->linesize[1];
	memcpy(pEntity->pU, frame->data[1], frame->linesize[1] * pEntity->height/2);
	pEntity->pV = new unsigned char[frame->linesize[2] * pEntity->height/2];
	pEntity->stride_v = frame->linesize[2];
	memcpy(pEntity->pV, frame->data[2], frame->linesize[2] * pEntity->height/2);

	return pEntity;

}

void EditorEntityRelease(EditorEntity** pEntity)
{
	if (*pEntity)
	{
		SAFE_DELETE_ARRAY((*pEntity)->pY);
		SAFE_DELETE_ARRAY((*pEntity)->pU);
		SAFE_DELETE_ARRAY((*pEntity)->pV);
	}
	SAFE_DELETE(*pEntity);
}


AudioEntity* AudioEntityCreate( AVFrame* frame ,double time_stemp)
{
	AudioEntity* entity = new AudioEntity();

	uint8_t** pAudio = new uint8_t*[frame->channels];
	for(int i = 0 ; i < frame->channels; i++)
	{
		if(frame->extended_data[i])
		{
			pAudio[i] = new uint8_t[frame->linesize[0]];
			memcpy(pAudio[i],frame->extended_data[i],sizeof(uint8_t)*frame->linesize[0]);
		}
		else
		{
			pAudio[i] = NULL;
		}
	}
	entity->pts = time_stemp;
	entity->data = pAudio;
	entity->channels = frame->channels;
	entity->nb_samples = frame->nb_samples;
	return entity;
}

void AudioEntityRelease( AudioEntity** pEntity )
{
	if(*pEntity == NULL)
	{
		return ;
	}
	for(int i = 0 ; i < (*pEntity)->channels; i++)
	{
		if((*pEntity)->data[i])
		{
			SAFE_DELETE_ARRAY((*pEntity)->data[i]);
		}
	}
	SAFE_DELETE_ARRAY((*pEntity)->data);
	SAFE_DELETE(*pEntity);
}


static int SearchPosition(vector<AudioEntity*>& vects,double time_stemp)
{
	for(int i = 0 ; i < vects.size(); i++)
	{
		if(vects[i]->pts > time_stemp || fabs(vects[i]->pts - time_stemp) < 0.02f)
		{
			return i;
		}
	}
	return vects.size()-1;
}

static int SearchPosition(vector<EditorEntity*>& vects,double time_stemp)
{
	for(int i = 0 ; i < vects.size(); i++)
	{
		if(vects[i]->pts > time_stemp || fabs(vects[i]->pts - time_stemp) < 0.01f)
		{
			return i;
		}
	}
	return (int)vects.size()-1;
}

int CVideoEditer::sws_mode = SWS_FLAGS;

void CVideoEditer::SetVideoEditerSwsMode(int mode) {
    switch (mode) {
        case SWS_BICUBIC:
            sws_mode = SWS_BICUBIC;
            break;
        case SWS_AREA:
            sws_mode = SWS_AREA;
            break;
        case SWS_BILINEAR:
            sws_mode = SWS_BILINEAR;
            break;
        case SWS_FAST_BILINEAR:
            sws_mode = SWS_FAST_BILINEAR;
            break;
        default:
            LOGE("Mode %d is invalid", mode);
            break;
    }
	LOGW("When Mode change success!, Need to recreate a new VideoEditer to make setting work.");
}

void CVideoEditer::SetVideoEditerOutputSize(int width, int height) {
    MT_IMPORT_SIZE_W = width;
    MT_IMPORT_SIZE_H = height;
}

CVideoEditer::CVideoEditer()
{
	m_pFormatContext = NULL;
	m_pVideoStream = NULL;
	m_pVideoCodec = NULL;
	m_pAudioStream = NULL;
	m_pAudioCodec = NULL;
	m_YuvFrame = NULL;
	m_pSrcFrame = NULL;
	m_pRGBFrame = NULL;
	rgb_sws_ctx = NULL;
	yuv_sws_ctx = NULL;
	m_DstVideoWidth = -1;
	m_DstVideoHeight = -1;
	m_ExitAudio = false;
	m_Left = 0;
	m_Top = 0;
	m_KeyFrameWidth = 240;
	m_KeyFrameHeight = 240;
	m_video_stream_idx = -1;
	m_audio_stream_idx = -1;
	m_KeyFrameStep = 2.0;
    listener = nullptr;
	m_Rotate = MT_VIDEO_ROTATE_0;
}


CVideoEditer::~CVideoEditer()
{
	this->Close();
	SAFE_DELETE(listener);
}

void CVideoEditer::setProgressListener(VideoEditerProgressListener *listener) {
	SAFE_DELETE(this->listener);
    this->listener = listener;
}

VideoEditerProgressListener* CVideoEditer::getProgressListener() {
    return this->listener;
}

void CVideoEditer::abort() {
    if (any_working) {
        abort_request = true;
    } else {
        // do nothing.
    }
}

bool CVideoEditer::isAborted() {
    return !any_working;
}

int CVideoEditer::Open(const char* file)
{
	int ret = -1;
	av_register_all();
	avcodec_register_all();

	if (m_pFormatContext)
	{
		avformat_close_input(&m_pFormatContext);
		m_pFormatContext = NULL;
	}

	if ((ret = avformat_open_input(&m_pFormatContext, file, NULL, NULL)) < 0)
	{
		LOGE("Error: Could not open %s (%s)\n", file, makeErrorStr(ret));
		avformat_close_input(&m_pFormatContext);
		m_pFormatContext = NULL;
		return -1;
	}

	/* ªÒ»°“Ù ”∆µ¡˜–≈œ¢ */
	if ((ret = avformat_find_stream_info(m_pFormatContext, NULL)) < 0)
	{
		LOGE("Could not find stream information (%s)\n", makeErrorStr(ret));
		avformat_close_input(&m_pFormatContext);
		m_pFormatContext = NULL;
		return -1;
	}

	//open codec context.
	if ((ret = open_codec_context(&m_video_stream_idx, m_pFormatContext, &m_pVideoCodec, AVMEDIA_TYPE_VIDEO)) < 0)
	{
		LOGE("No exit video.\n");
		avformat_close_input(&m_pFormatContext);
		return -2;
	}
	if (m_video_stream_idx >= 0)
	{
		m_pVideoStream = m_pFormatContext->streams[m_video_stream_idx];
	}

	AVDictionaryEntry *tag = NULL;
	tag = av_dict_get(m_pVideoStream->metadata, "rotate", tag, 0);
	if (tag == NULL)
	{
		m_Rotate = MT_VIDEO_ROTATE_0;
	}
	else
	{
		int angle = atoi(tag->value);
		angle %= 360;
		if (angle == 90)
		{
			m_Rotate = MT_VIDEO_ROTATE_90;
		}
		else if (angle == 180)
		{
			m_Rotate = MT_VIDEO_ROTATE_180;
		}
		else if (angle == 270)
		{
			m_Rotate = MT_VIDEO_ROTATE_270;
		}
		else
		{
			m_Rotate = MT_VIDEO_ROTATE_0;
		}
	}


	if ((ret = open_codec_context(&m_audio_stream_idx, m_pFormatContext, &m_pAudioCodec, AVMEDIA_TYPE_AUDIO)) < 0)
	{
		LOGE("No exit audio.\n");
		ret = 0;
	}
	if (m_audio_stream_idx >= 0)
	{
		m_pAudioStream = m_pFormatContext->streams[m_audio_stream_idx];
		m_ExitAudio = true;
	}

	m_VideoWidth = m_pVideoCodec->width;
	m_VideoHeight = m_pVideoCodec->height;

	if (m_VideoWidth < 1 || m_VideoHeight < 1)
	{
		LOGE("warning : %s has error width :%d,height:%d", file, m_VideoWidth, m_VideoHeight);
		m_VideoWidth = m_VideoHeight = 1;
	}


	float vhRate = m_VideoWidth / (float)m_VideoHeight;
    float vhDstRate = MT_IMPORT_SIZE_W/(float)MT_IMPORT_SIZE_H;
    
    if (m_Rotate == MT_VIDEO_ROTATE_90 || m_Rotate == MT_VIDEO_ROTATE_270) {
        vhRate = 1/vhRate;
    }
    
	//º∆À„Àı∑≈∫ÛµƒøÌ∏ﬂ
	if (vhRate > vhDstRate) // 视频的宽高比大于屏幕的宽高比
	{
		m_DstVideoWidth = MT_IMPORT_SIZE_W;
        m_DstVideoHeight = m_DstVideoWidth/vhRate;
        while((MT_IMPORT_SIZE_H - m_DstVideoHeight)%4 != 0)
        {
            m_DstVideoHeight--;
        }
	}
	else
	{
		m_DstVideoHeight = MT_IMPORT_SIZE_H;
		m_DstVideoWidth = m_DstVideoHeight*vhRate;
        while((MT_IMPORT_SIZE_W - m_DstVideoWidth)%4 != 0)
        {
            m_DstVideoWidth--;
        }
	}

	if (m_DstVideoWidth <= 1 || m_DstVideoHeight <= 1)
	{
		LOGE("invalid video.");
		return -1;
	}

	if (m_pVideoCodec->pix_fmt != AV_PIX_FMT_NONE && (m_pVideoCodec->pix_fmt != AV_PIX_FMT_YUV420P ||
		m_VideoWidth != m_DstVideoWidth || m_VideoHeight != m_DstVideoHeight))
	{
		if (yuv_sws_ctx)
		{
			sws_freeContext(yuv_sws_ctx);
			yuv_sws_ctx = NULL;
		}
        if (m_YuvFrame)
        {
            av_frame_free(&m_YuvFrame);
            m_YuvFrame = NULL;
        }
        //…Í«Î“ª’≈YUV÷°
        m_YuvFrame = av_frame_alloc();
        
        if (m_Rotate == MT_VIDEO_ROTATE_90 || m_Rotate == MT_VIDEO_ROTATE_270) {
            yuv_sws_ctx = sws_getContext(m_VideoWidth,
                                         m_VideoHeight,
                                         m_pVideoCodec->pix_fmt,
                                         m_DstVideoHeight,
                                         m_DstVideoWidth,
                                         AV_PIX_FMT_YUV420P,
                                         sws_mode, NULL, NULL, NULL);
            {
                int numBytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, m_DstVideoHeight, m_DstVideoWidth, 1);
                uint8_t *buffer = (uint8_t *)av_malloc_array(1, numBytes);
                av_image_fill_arrays(m_YuvFrame->data, m_YuvFrame->linesize, buffer, AV_PIX_FMT_YUV420P, m_DstVideoHeight, m_DstVideoWidth, 1);
            }
            
        } else {
            yuv_sws_ctx = sws_getContext(m_VideoWidth,
                                         m_VideoHeight,
                                         m_pVideoCodec->pix_fmt,
                                         m_DstVideoWidth,
                                         m_DstVideoHeight,
                                         AV_PIX_FMT_YUV420P,
                                         sws_mode, NULL, NULL, NULL);
            {
                int numBytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, m_DstVideoHeight, m_DstVideoWidth, 1);
                uint8_t *buffer = (uint8_t *)av_malloc_array(1, numBytes);
                av_image_fill_arrays(m_YuvFrame->data, m_YuvFrame->linesize, buffer, AV_PIX_FMT_YUV420P, m_DstVideoWidth, m_DstVideoHeight, 1);
            }
        }



	}

	if (m_pSrcFrame)
	{
		av_frame_free(&m_pSrcFrame);
		m_pSrcFrame = NULL;
	}
	m_pSrcFrame = av_frame_alloc();

	av_dump_format(m_pFormatContext, 0, file, 0);

	return ret;
}

int CVideoEditer::SetLeftTop(int left, int top)
{
	if (m_pFormatContext == NULL)
	{
		LOGE("No Open any Video");
		return -1;
	}
	//’‚¿Ô…Ë÷√µƒ « ”∆µ–˝◊™∫Û’˝≥£œ‘ æµƒleft∫Õtop.
	//À˘“‘–Ë“™◊ˆœ‡”¶µƒ◊™ªª

	if (m_Rotate == MT_VIDEO_ROTATE_90)
	{
		// ”∆µ–˝◊™¡À90∂»;
		if (m_DstVideoWidth > m_DstVideoHeight)
		{
			m_Top = 0;
			m_Left = top;
		}
		else
		{
			m_Left = 0;
			m_Top = m_DstVideoHeight - left - m_DstVideoWidth;
		}
	}
	else if (m_Rotate == MT_VIDEO_ROTATE_180)
	{
		// ”∆µ–˝◊™¡À180∂»
		if (m_DstVideoWidth > m_DstVideoHeight)
		{
			m_Top = 0;
			m_Left = m_DstVideoWidth - m_DstVideoHeight - left;
		}
		else
		{
			m_Left = 0;
			m_Top = m_DstVideoHeight - m_DstVideoWidth - top;
		}
	}
	else if (m_Rotate == MT_VIDEO_ROTATE_270)
	{
		if (m_DstVideoWidth > m_DstVideoHeight)
		{
			m_Top = 0;
			m_Left = m_DstVideoWidth - m_DstVideoHeight - top;
		}
		else
		{
			m_Left = 0;
			m_Top = left;
		}
	}
	else
	{
		// ”∆µ’˝≥£;
		m_Left = left;
		m_Top = top;
	}

	if (m_Left & 1)
	{
		//≈≈≥˝∆Ê ˝øÌ;
		m_Left++;
		if (m_Left + MT_IMPORT_SIZE_W > m_DstVideoWidth && m_Left + MT_IMPORT_SIZE_W - 2 <= m_DstVideoWidth)
		{
			m_Left -= 2;
		}
	}

	if (m_Left + MT_IMPORT_SIZE_W > m_DstVideoWidth || m_Top + MT_IMPORT_SIZE_H > m_DstVideoHeight)
	{
		LOGE("out size!! %d,%d,---%d,---%d,  %d,%d", m_Left, m_Top, MT_IMPORT_SIZE_W, MT_IMPORT_SIZE_H, m_DstVideoWidth, m_DstVideoHeight);
		return -1;
	}
	return 1;
}


unsigned char* CVideoEditer::CropI420(unsigned char* pY, int stride_y, unsigned char* pU, int stride_u,
	unsigned char* pV, int stride_v, int width, int height)
{
	if (pY == NULL || pV == NULL || pU == NULL || width <= 0 || height <= 0)
	{
		return NULL;
	}
	int src_stride_y = stride_y;
	int src_stride_u = stride_u;
	int src_stride_v = stride_v;

	unsigned char* dst = (unsigned char*)aligned_malloc(3 * MT_IMPORT_SIZE_W*MT_IMPORT_SIZE_H / 2, 64);
	unsigned char* src_y = pY + m_Top*src_stride_y + m_Left;
	unsigned char* src_u = pU + (m_Top / 2)*src_stride_u + m_Left / 2;
	unsigned char* src_v = pV + (m_Top / 2)*src_stride_v + m_Left / 2;

	int dst_stride_y = MT_IMPORT_SIZE_W;
	int dst_stride_u = MT_IMPORT_SIZE_W / 2;
	int dst_stride_v = MT_IMPORT_SIZE_W / 2;

	unsigned char* dst_y = dst;
	unsigned char* dst_u = dst + MT_IMPORT_SIZE_W*MT_IMPORT_SIZE_H;
	unsigned char* dst_v = dst_u + MT_IMPORT_SIZE_W*MT_IMPORT_SIZE_H / 4;

	for (int i = 0; i < MT_IMPORT_SIZE_H; i++)
	{
		memcpy(dst_y, src_y, dst_stride_y);
		src_y += src_stride_y;
		dst_y += dst_stride_y;
		if (i & 1)
		{
			memcpy(dst_u, src_u, dst_stride_u);
			memcpy(dst_v, src_v, dst_stride_v);
			dst_u += dst_stride_u;
			dst_v += dst_stride_v;
			src_u += src_stride_u;
			src_v += src_stride_v;
		}
	}

	return dst;
}


//ÃÓ≥‰∫⁄∞◊±ﬂøÚ
void CVideoEditer::FillVideoFrame(unsigned char* pFillRes,
                                  uint8_t* pDstY, int dst_stride_y,
                                  uint8_t* pDstU, int dst_stride_u,
                                  uint8_t* pDstV, int dst_stride_v,
                                  int dstWidth, int dstHeight,
                                  unsigned char cY,unsigned char cB,unsigned char cR) {
   	unsigned char* pSrcY = pFillRes;
    unsigned char* pSrcU = pFillRes + MT_IMPORT_SIZE_W*MT_IMPORT_SIZE_H;
    unsigned char* pSrcV = pSrcU + MT_IMPORT_SIZE_W*MT_IMPORT_SIZE_H/4;
    memset(pSrcY,cY,MT_IMPORT_SIZE_W*MT_IMPORT_SIZE_H);
    memset(pSrcU,cB,MT_IMPORT_SIZE_W*MT_IMPORT_SIZE_H/4);
    memset(pSrcV,cR,MT_IMPORT_SIZE_W*MT_IMPORT_SIZE_H/4);
    
    if(dstWidth == MT_IMPORT_SIZE_W && dstHeight == MT_IMPORT_SIZE_H)
    {
        //÷±Ω”øΩ±¥
        for(int i = 0 ; i < MT_IMPORT_SIZE_H ; i ++)
        {
            memcpy(pSrcY,pDstY,MT_IMPORT_SIZE_W*sizeof(unsigned char));
            pSrcY += MT_IMPORT_SIZE_W;
            pDstY += dst_stride_y;
            if(i&1)
            {
                memcpy(pSrcU,pDstU,MT_IMPORT_SIZE_W/2*sizeof(unsigned char));
                pSrcU += MT_IMPORT_SIZE_W/2;
                pDstU += dst_stride_u;
                memcpy(pSrcV,pDstV,MT_IMPORT_SIZE_W/2*sizeof(unsigned char));
                pSrcV += MT_IMPORT_SIZE_W/2;
                pDstV += dst_stride_v;
            }
        }
        return ;
    }
    
    if(dstWidth < MT_IMPORT_SIZE_W)
    {
        int offset = (MT_IMPORT_SIZE_W - dstWidth)/2;
        
        for(int i = 0 ; i < MT_IMPORT_SIZE_H ; i ++)
        {
            memcpy(pSrcY + i*MT_IMPORT_SIZE_W + offset,pDstY + dst_stride_y*i,dstWidth*sizeof(unsigned char));
        }
        for(int i = 0 ; i < MT_IMPORT_SIZE_H/2 ; i ++)
        {
            memcpy(pSrcU + i*MT_IMPORT_SIZE_W/2 + offset/2,pDstU + dst_stride_u*i,dstWidth/2*sizeof(unsigned char));
            memcpy(pSrcV + i*MT_IMPORT_SIZE_W/2 + offset/2,pDstV + dst_stride_v*i,dstWidth/2*sizeof(unsigned char));
        }
    }
    else
    {
        int offset =  (MT_IMPORT_SIZE_H - dstHeight)/2;
        pSrcY += offset*MT_IMPORT_SIZE_W;
        pSrcU += (offset/2)*MT_IMPORT_SIZE_W/2;
        pSrcV += (offset/2)*MT_IMPORT_SIZE_W/2;
        int ending = MT_IMPORT_SIZE_H-offset;
        for(int  i = offset;i < ending;i++)
        {
            memcpy(pSrcY,pDstY,MT_IMPORT_SIZE_W*sizeof(unsigned char));
            pSrcY += MT_IMPORT_SIZE_W;
            pDstY += dst_stride_y;
            if(i&1)
            {
                memcpy(pSrcU,pDstU,MT_IMPORT_SIZE_W/2*sizeof(unsigned char));
                pSrcU += MT_IMPORT_SIZE_W/2;
                pDstU += dst_stride_u;
                memcpy(pSrcV,pDstV,MT_IMPORT_SIZE_W/2*sizeof(unsigned char));
                pSrcV += MT_IMPORT_SIZE_W/2;
                pDstV += dst_stride_v;
            }
        }
    }
}

void CVideoEditer::FillVideoFrame(unsigned char* pFillRes,AVFrame * frame,unsigned char cY,unsigned char cB,unsigned char cR)
{
	int dstWidth = frame->width;
	int dstHeight = frame->height;

	unsigned char* pDstY = frame->data[0];
	unsigned char* pDstU = frame->data[1];
	unsigned char* pDstV = frame->data[2];
	int dst_stride_y = frame->linesize[0];
	int dst_stride_u = frame->linesize[1];
	int dst_stride_v = frame->linesize[2];
    
    FillVideoFrame(pFillRes, pDstY, dst_stride_y, pDstU, dst_stride_u, pDstV, dst_stride_v, dstWidth, dstHeight, cY, cB, cR);
}

int CVideoEditer::SetKeyFrameSize(int width, int height)
{
	if (width <= 0 || (width & 1) || height <= 0 || (height & 1))
	{
		LOGE("invalid w:%d,h:%d", width, height);
		return -1;
	}

	m_KeyFrameWidth = width;
	m_KeyFrameHeight = height;
	if (rgb_sws_ctx)
	{
		sws_freeContext(rgb_sws_ctx);
		rgb_sws_ctx = NULL;
	}

	rgb_sws_ctx = sws_getContext(
		m_VideoWidth,
		m_VideoHeight,
		m_pVideoCodec->pix_fmt,
		m_KeyFrameWidth,
		m_KeyFrameHeight,
		AV_PIX_FMT_RGBA,
		sws_mode, NULL, NULL, NULL);

	if (m_pRGBFrame)
	{
		if (m_pRGBFrame->data[0])
		{
			av_free(m_pRGBFrame->data[0]);
			m_pRGBFrame->data[0] = NULL;
		}
		av_frame_free(&m_pRGBFrame);
		m_pRGBFrame = NULL;
	}

	m_pRGBFrame = av_frame_alloc();
	{
		int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, m_KeyFrameWidth, m_KeyFrameHeight, 1);
		uint8_t *buffer = (uint8_t *)av_malloc_array(1, numBytes);
        int len = av_image_fill_arrays(m_pRGBFrame->data, m_pRGBFrame->linesize, buffer, AV_PIX_FMT_RGBA, m_KeyFrameWidth, m_KeyFrameHeight, 1);
	}
	return 0;
}

int CVideoEditer::GetAllKeyFrame(const char* floder)
{
	if (m_pFormatContext == NULL)
	{
		LOGE("no open any file or open file fail");
		return -1;
	}
	int ret = -1;
	int nGotFrame = 0;
	int frameFinished = 0;
	AVPacket packet = { 0 };
	av_init_packet(&packet);

	ret = avformat_seek_file(m_pFormatContext, -1, INT64_MIN, 0, INT64_MAX, 0);
	avcodec_flush_buffers(m_pVideoCodec);
	unsigned char* pResARGB = NULL;
	if (m_Rotate != MT_VIDEO_ROTATE_0)
	{
		pResARGB = new unsigned char[m_KeyFrameHeight*m_KeyFrameWidth*4];
	}
	double nPreSaveStamp = -m_KeyFrameStep;
	bool startKey = false;
	while (av_read_frame(m_pFormatContext, &packet) >= 0)
	{
		if (packet.stream_index == m_video_stream_idx && (!startKey||(startKey && packet.flags&AV_PKT_FLAG_KEY)))
		{
			ret = avcodec_decode_video2(m_pVideoCodec, m_pSrcFrame, &frameFinished, &packet);
			if (frameFinished)
			{
				if (m_pSrcFrame->key_frame)
				{
					int64_t nPTS = av_frame_get_best_effort_timestamp(m_pSrcFrame);
					double time_stamp = nPTS* av_q2d(m_pVideoStream->time_base);

					sws_scale(rgb_sws_ctx, m_pSrcFrame->data, m_pSrcFrame->linesize, 0, m_VideoHeight, m_pRGBFrame->data, m_pRGBFrame->linesize);
					int saveWidth = m_KeyFrameWidth;
					int saveHeight = m_KeyFrameHeight;
					unsigned char* saveData = NULL;
					if (m_Rotate == MT_VIDEO_ROTATE_90)
					{
						ARGBRotate(m_pRGBFrame->data[0], saveWidth * 4, pResARGB, saveHeight * 4, saveWidth, saveHeight, kRotate90);
						int tmp = saveWidth;
						saveWidth = saveHeight;
						saveHeight = tmp;
						saveData = pResARGB;
					}
					else if (m_Rotate == MT_VIDEO_ROTATE_270)
					{
						ARGBRotate(m_pRGBFrame->data[0], saveWidth * 4, pResARGB, saveHeight * 4, saveWidth, saveHeight, kRotate270);
						int tmp = saveWidth;
						saveWidth = saveHeight;
						saveHeight = tmp;
						saveData = pResARGB;
					}
					else if (m_Rotate == MT_VIDEO_ROTATE_180)
					{
						ARGBRotate(m_pRGBFrame->data[0], saveWidth * 4, pResARGB, saveWidth * 4, saveWidth, saveHeight, kRotate180);
						saveData = pResARGB;
					}
					else
					{
						saveData = m_pRGBFrame->data[0];
					}
					int strLength=strlen(floder)+32;
					char* szFileName=new char[strLength];
					memset(szFileName,0,strLength);
					sprintf(szFileName, "%s/%d.ppm",floder,nGotFrame);			
					SaveFrameARGB(saveData, saveWidth, saveHeight, szFileName,time_stamp);
					delete[] szFileName;
					nGotFrame++;
				}
				startKey = true;

			}
		}
	}

	if (m_pVideoCodec)
	{
		for (;;)
		{
			av_init_packet(&packet);
			ret = avcodec_decode_video2(m_pVideoCodec, m_pSrcFrame, &frameFinished, &packet);

			if (ret < 0)
			{
				av_packet_unref(&packet);
				break;
			}


			if (frameFinished) {
				if (m_pSrcFrame->key_frame)
				{
					int64_t nPTS = av_frame_get_best_effort_timestamp(m_pSrcFrame);
					double time_stamp = nPTS* av_q2d(m_pVideoStream->time_base);
					sws_scale(rgb_sws_ctx, m_pSrcFrame->data, m_pSrcFrame->linesize, 0, m_VideoHeight, m_pRGBFrame->data, m_pRGBFrame->linesize);
					int saveWidth = m_KeyFrameWidth;
					int saveHeight = m_KeyFrameHeight;
					unsigned char* saveData = NULL;
					if (m_Rotate == MT_VIDEO_ROTATE_90)
					{
						ARGBRotate(m_pRGBFrame->data[0], saveWidth * 4, pResARGB, saveHeight * 4, saveWidth, saveHeight, kRotate90);
						int tmp = saveWidth;
						saveWidth = saveHeight;
						saveHeight = tmp;
						saveData = pResARGB;
					}
					else if (m_Rotate == MT_VIDEO_ROTATE_270)
					{
						ARGBRotate(m_pRGBFrame->data[0], saveWidth * 4, pResARGB, saveHeight * 4, saveWidth, saveHeight, kRotate270);
						int tmp = saveWidth;
						saveWidth = saveHeight;
						saveHeight = tmp;
						saveData = pResARGB;
					}
					else if (m_Rotate == MT_VIDEO_ROTATE_180)
					{
						ARGBRotate(m_pRGBFrame->data[0], saveWidth * 4, pResARGB, saveWidth * 4, saveWidth, saveHeight, kRotate180);
						saveData = pResARGB;
					}
					else
					{
						saveData = m_pRGBFrame->data[0];
					}
					int strLength=strlen(floder)+32;
					char* szFileName=new char[strLength];
					memset(szFileName,0,strLength);
					sprintf(szFileName, "%s/%d.ppm",floder,nGotFrame);			
					SaveFrameARGB(saveData, saveWidth, saveHeight, szFileName,time_stamp);
					delete[] szFileName;
					nGotFrame++;
				}
				av_packet_unref(&packet);
			}
			else
			{
				av_packet_unref(&packet);
				break;
			}
		}
	}

	SAFE_DELETE_ARRAY(pResARGB);

	return nGotFrame;

}

int CVideoEditer::getVideoRotation() {
    return (int)m_Rotate;
}

double CVideoEditer::GetVideoDuration()
{
    if (m_pFormatContext == NULL)
    {
        LOGE("No any video is open!");
        return 0.0;
    }
    // delete by Javan .2015.9.10 fix video duration not right
//    if (m_pVideoStream && m_pVideoStream->duration != AV_NOPTS_VALUE)
//    {
//        double divideFactor;
//        divideFactor = (double)1 / av_q2d(m_pVideoStream->time_base);
//        double durationOfVideo = (double)m_pVideoStream->duration / divideFactor;
//        return durationOfVideo;
//    }

	if (m_pFormatContext->duration != AV_NOPTS_VALUE)
	{
		int hours, mins, secs, us;
		int64_t duration = m_pFormatContext->duration;
		secs = duration / AV_TIME_BASE;
		us = duration % AV_TIME_BASE;
		mins = secs / 60;
		secs %= 60;
		hours = mins / 60;
		mins %= 60;
		LOGE("  Duration: %02d:%02d:%02d.%02d\n", hours, mins, secs, (100 * us) / AV_TIME_BASE);
		m_Duration = hours*60.0*60.0 + mins*60.0 + secs + (double)us / AV_TIME_BASE;
		return m_Duration;
	}
	else
	{
		LOGE("Could not get video duration (N/A).\n");
		return 0.0;
	}

	return 0.0;
}


double CVideoEditer::GetAudioDuration()
{
	if (m_pAudioStream == NULL)
	{
		LOGE("No any audio is open or no have audio stream!");
		return 0.0;
	}

	if (m_pAudioStream->duration != AV_NOPTS_VALUE)
	{
		double divideFactor;
		divideFactor = (double)1 / av_q2d(m_pAudioStream->time_base);
		double durationOfAudio = (double)m_pAudioStream->duration / divideFactor;
		return durationOfAudio;
	}
	return 0.0;
}


int CVideoEditer::GetVideoWidth()
{
	if (m_pFormatContext == NULL || m_pVideoStream == NULL)
	{
		LOGE("No open any video or no video stream.");
		return 0;
	}
	return m_VideoWidth;
}

int CVideoEditer::GetVideoHeight()
{
	if (m_pFormatContext == NULL || m_pVideoStream == NULL)
	{
		LOGE("No open any video or no video stream.");
		return 0;
	}
	return m_VideoHeight;
}

int CVideoEditer::getVideoBitrate()
{
    if (m_pFormatContext == NULL || m_pVideoStream == NULL)
    {
        LOGE("No open any video or no video stream.");
        return 0;
    }
    return m_pFormatContext->bit_rate;
}


int CVideoEditer::GetShowWidth()
{
	if (m_pFormatContext == NULL || m_pVideoStream == NULL)
	{
		LOGE("No open any video or no video stream.");
		return 0;
	}
	if (m_Rotate == MT_VIDEO_ROTATE_90 || m_Rotate == MT_VIDEO_ROTATE_270)
	{
		return m_VideoHeight;
	}
	return m_VideoWidth;
}

int CVideoEditer::GetShowHeight()
{
	if (m_pFormatContext == NULL || m_pVideoStream == NULL)
	{
		LOGE("No open any video or no video stream.");
		return 0;
	}
	if (m_Rotate == MT_VIDEO_ROTATE_90 || m_Rotate == MT_VIDEO_ROTATE_270)
	{
		return m_VideoWidth;
	}
	return m_VideoHeight;
}

int CVideoEditer::CutVideo(const char* dstFile,double startTime, double endTime,
                                   unsigned char r,unsigned char g,unsigned char b)
{
    unsigned char colorY = RGBToY(r,g,b);//r *  0.299000f + g *  0.587000f + b *  0.114000f;
    unsigned char colorU = RGBToU(r,g,b);;//r * -0.168736f + g * -0.331264f + b *  0.500000f + 128;
    unsigned char colorV = RGBToV(r,g,b);;//r *  0.500000f + g * -.418688f  + b *  -0.081312f + 128;
    
	if (m_pFormatContext == NULL)
	{
		LOGE("No any video is open!");
		return -1;
	}

    any_working = true;
    
	double duration = this->GetVideoDuration() + 0.01;

    if (endTime > duration && (endTime - 0.1) < duration) {
        endTime -= 0.1;
    }
    
	if (startTime >= endTime || startTime > duration || endTime > duration)
	{
		LOGE("invalid parameter. start:%f,end:%f,duration:%f", startTime, endTime, duration);
		return -1;
	}

	CFrameRecorder recorder;
	recorder.Open(dstFile, MT_IMPORT_SIZE_W, MT_IMPORT_SIZE_H);
	recorder.SetupCropRegion(0, 0, MT_IMPORT_SIZE_W, MT_IMPORT_SIZE_H,0);
	if (m_ExitAudio)
	{
		recorder.SetupAudio(m_pAudioCodec->channels, m_pAudioCodec->sample_rate, m_pAudioCodec->sample_fmt);
	}
	else
	{
		recorder.SetupAudio(1, 44100, AV_SAMPLE_FMT_S16);
	}

	recorder.SetupAudioAligen(true);

	recorder.Start();

	int ret = -1;
	int frameFinished = 0;
	int64_t video_start_pts = (startTime*(m_pVideoStream->time_base.den)) / (m_pVideoStream->time_base.num) + 0.5;
	int64_t video_end_pts = (endTime*(m_pVideoStream->time_base.den)) / (m_pVideoStream->time_base.num) + 0.5;
	int64_t audio_start_pts = 0;
	int64_t audio_end_pts = 0;
	if (m_ExitAudio)
	{
		audio_start_pts = (startTime*(m_pAudioStream->time_base.den)) / (m_pAudioStream->time_base.num) + 0.5;
		audio_end_pts = (endTime*(m_pAudioStream->time_base.den)) / (m_pAudioStream->time_base.num) + 0.5;
	}

    double cut_total = endTime - startTime + 0.1;
    
	bool nVideoFlag = false;// ”∆µ «∑Ò“—æ≠Ωÿ»°ÕÍ
	bool nAudioFlag = false;//“Ù∆µ «∑Ò“—æ≠Ωÿ»°ÕÍ
	double last_time_stamp = 0.0;
	//---------------seek-------------------------------;
	if (av_seek_frame(m_pFormatContext, m_video_stream_idx, video_start_pts, AVSEEK_FLAG_BACKWARD) < 0)
	{
		LOGE("Failed to seek Video \n");
	}

	if (m_ExitAudio)
	{
		if (av_seek_frame(m_pFormatContext, m_audio_stream_idx, audio_start_pts, AVSEEK_FLAG_BACKWARD) < 0)
		{
			LOGE("Failed to seek Audio \n");
		}
	}
	avcodec_flush_buffers(m_pVideoCodec);
	//------------------end seek----------------------;

	unsigned char* pResYuvData = NULL;
    unsigned char* pFillData = NULL;
	if (m_Rotate != MT_VIDEO_ROTATE_0)
	{
		pResYuvData = new uint8_t[m_DstVideoWidth*m_DstVideoHeight * 3 / 2];
	}
    
    pFillData = new unsigned char[MT_IMPORT_SIZE_W*MT_IMPORT_SIZE_H*3/2];
    
	LOGE("in cut %p", listener);
    if (listener) {
        listener->videoEditerProgressBegan(this);
    }
    
	AVPacket packet = { 0 };
	av_init_packet(&packet);

	while (((ret = av_read_frame(m_pFormatContext, &packet)) >= 0 || ret < 0) && !abort_request)
	{
        if (ret < 0 && m_pVideoCodec && !nVideoFlag)
        {
            av_init_packet(&packet);
            packet.stream_index = m_video_stream_idx;
        } else if ( ret < 0 && nVideoFlag ) {
            // all video has done.
            break;
        } else {
            // do nothing.
        }
        
		if (packet.stream_index == m_video_stream_idx && !nVideoFlag)
		{
			ret = avcodec_decode_video2(m_pVideoCodec, m_pSrcFrame, &frameFinished, &packet);
			if (frameFinished)
			{
				int64_t nPTS = av_frame_get_best_effort_timestamp(m_pSrcFrame);

				if (nPTS < video_start_pts)
				{
					av_packet_unref(&packet);
					av_init_packet(&packet);
					continue;
				}
				if(nPTS > video_end_pts)
				{
					nVideoFlag = true;
					if (nAudioFlag || m_ExitAudio == false)
					{
						break;
					}
					av_packet_unref(&packet);
					av_init_packet(&packet);
					continue;
				}

				double time_stemp = (nPTS - video_start_pts)* av_q2d(m_pVideoStream->time_base);
				last_time_stamp = time_stemp;
				if (yuv_sws_ctx)
				{
					//–Ë“™◊™≥…YUV420P
					ret = sws_scale(yuv_sws_ctx, m_pSrcFrame->data, m_pSrcFrame->linesize, 0, m_pSrcFrame->height, m_YuvFrame->data, m_YuvFrame->linesize);
//                    ret = I420Scale(m_pSrcFrame->data[0], m_pSrcFrame->linesize[0], m_pSrcFrame->data[1], m_pSrcFrame->linesize[1], m_pSrcFrame->data[2], m_pSrcFrame->linesize[2], m_pSrcFrame->width, m_pSrcFrame->height, m_YuvFrame->data[0], m_YuvFrame->linesize[0], m_YuvFrame->data[1], m_YuvFrame->linesize[1], m_YuvFrame->data[2], m_YuvFrame->linesize[2], m_DstVideoWidth, m_DstVideoHeight, kFilterBilinear);
					if (ret < 0)
					{
						LOGE("error in sws_scale.");
						return -1;
					}
                    
                    byte* pDstY = pFillData;
                    byte* pDstU = pFillData + MT_IMPORT_SIZE_W*MT_IMPORT_SIZE_H;
                    byte* pDstV = pDstU + MT_IMPORT_SIZE_W*MT_IMPORT_SIZE_H / 4;
                    
                    pDstY = m_YuvFrame->data[0];
                    pDstU = m_YuvFrame->data[1];
                    pDstV = m_YuvFrame->data[2];
                    int dst_stride_y = m_YuvFrame->linesize[0];
                    int dst_stride_u = m_YuvFrame->linesize[1];
                    int dst_stride_v = m_YuvFrame->linesize[2];
                    
                    byte* pResY = pDstY;
                    byte* pResU = pDstU;
                    byte* pResV = pDstV;

					if (m_Rotate == MT_VIDEO_ROTATE_90)
					{
						pResY = pResYuvData;
						pResU = pResYuvData + m_DstVideoWidth*m_DstVideoHeight;
						pResV = pResU +  m_DstVideoWidth*m_DstVideoHeight/4;

						I420Rotate(pDstY, dst_stride_y, pDstU, dst_stride_u,
							pDstV, dst_stride_v, pResY, m_DstVideoWidth,
							pResU, m_DstVideoWidth / 2, pResV, m_DstVideoWidth / 2,
							m_DstVideoHeight, m_DstVideoWidth, kRotate90);
					}
					else if (m_Rotate == MT_VIDEO_ROTATE_180)
					{
                        pResY = pResYuvData;
                        pResU = pResYuvData + m_DstVideoWidth*m_DstVideoHeight;
                        pResV = pResU +  m_DstVideoWidth*m_DstVideoHeight/4;
                        
                        I420Rotate(pDstY, dst_stride_y, pDstU, dst_stride_u,
                                   pDstV, dst_stride_v, pResY, m_DstVideoWidth,
                                   pResU, m_DstVideoWidth / 2, pResV, m_DstVideoWidth / 2,
                                   m_DstVideoWidth, m_DstVideoHeight, kRotate90);
					}
					else if (m_Rotate == MT_VIDEO_ROTATE_270)
					{
                        pResY = pResYuvData;
                        pResU = pResYuvData + m_DstVideoWidth*m_DstVideoHeight;
                        pResV = pResU +  m_DstVideoWidth*m_DstVideoHeight/4;
                        
                        I420Rotate(pDstY, dst_stride_y, pDstU, dst_stride_u,
                                   pDstV, dst_stride_v, pResY, m_DstVideoWidth,
                                   pResU, m_DstVideoWidth / 2, pResV, m_DstVideoWidth / 2,
                                   m_DstVideoHeight, m_DstVideoWidth, kRotate90);
                    } else {
                        
                    }
                    
                    FillVideoFrame(pFillData,
                                   pResY, m_DstVideoWidth,
                                   pResU, m_DstVideoWidth/2,
                                   pResV, m_DstVideoWidth/2,
                                   m_DstVideoWidth, m_DstVideoHeight,
                                   colorY,colorU,colorV);
                    
                    pDstY = pFillData;
                    pDstU = pFillData + MT_IMPORT_SIZE_W*MT_IMPORT_SIZE_H;
                    pDstV = pDstU + MT_IMPORT_SIZE_W*MT_IMPORT_SIZE_H / 4;
                    
                    pResY = pDstY;
                    pResU = pDstU;
                    pResV = pDstV;
                    
					recorder.RecordI420(pResY, MT_IMPORT_SIZE_W, pResU, MT_IMPORT_SIZE_W / 2,
						pResV, MT_IMPORT_SIZE_W / 2, MT_IMPORT_SIZE_W, MT_IMPORT_SIZE_H, time_stemp);
                    
                    if (listener) {
                        listener->videoEditerProgressChanged(this, time_stemp, cut_total);
                    }
				}
				else
				{
					unsigned char* dst = this->CropI420(m_pSrcFrame->data[0], m_pSrcFrame->linesize[0],
						m_pSrcFrame->data[1], m_pSrcFrame->linesize[1],
						m_pSrcFrame->data[2], m_pSrcFrame->linesize[2], m_DstVideoWidth, m_DstVideoHeight);
					byte* pDstY = dst;
					byte* pDstU = dst + MT_IMPORT_SIZE_W*MT_IMPORT_SIZE_H;
					byte* pDstV = pDstU + MT_IMPORT_SIZE_W*MT_IMPORT_SIZE_H / 4;

					byte* pResY = pDstY;
					byte* pResU = pDstU;
					byte* pResV = pDstV;


					if (m_Rotate == MT_VIDEO_ROTATE_90)
					{
						pResY = pResYuvData;
						pResU = pResYuvData + MT_IMPORT_SIZE_W*MT_IMPORT_SIZE_H;
						pResV = pResU + MT_IMPORT_SIZE_W*MT_IMPORT_SIZE_H / 4;

						I420Rotate(pDstY, MT_IMPORT_SIZE_W, pDstU, MT_IMPORT_SIZE_W / 2,
							pDstV, MT_IMPORT_SIZE_W / 2, pResY, MT_IMPORT_SIZE_W,
							pResU, MT_IMPORT_SIZE_W / 2, pResV, MT_IMPORT_SIZE_W / 2,
							MT_IMPORT_SIZE_W, MT_IMPORT_SIZE_H, kRotate90);
					}
					else if (m_Rotate == MT_VIDEO_ROTATE_180)
					{
						pResY = pResYuvData;
						pResU = pResYuvData + MT_IMPORT_SIZE_W*MT_IMPORT_SIZE_H;
						pResV = pResU + MT_IMPORT_SIZE_W*MT_IMPORT_SIZE_H / 4;

						I420Rotate(pDstY, MT_IMPORT_SIZE_W, pDstU, MT_IMPORT_SIZE_W / 2,
							pDstV, MT_IMPORT_SIZE_W / 2, pResY, MT_IMPORT_SIZE_W,
							pResU, MT_IMPORT_SIZE_W / 2, pResV, MT_IMPORT_SIZE_W / 2,
							MT_IMPORT_SIZE_W, MT_IMPORT_SIZE_H, kRotate180);
					}
					else if (m_Rotate == MT_VIDEO_ROTATE_270)
					{
						pResY = pResYuvData;
						pResU = pResYuvData + MT_IMPORT_SIZE_W*MT_IMPORT_SIZE_H;
						pResV = pResU + MT_IMPORT_SIZE_W*MT_IMPORT_SIZE_H / 4;

						I420Rotate(pDstY, MT_IMPORT_SIZE_W, pDstU, MT_IMPORT_SIZE_W / 2,
							pDstV, MT_IMPORT_SIZE_W / 2, pResY, MT_IMPORT_SIZE_W,
							pResU, MT_IMPORT_SIZE_W / 2, pResV, MT_IMPORT_SIZE_W / 2,
							MT_IMPORT_SIZE_W, MT_IMPORT_SIZE_H, kRotate270);
					}

					recorder.RecordI420(pResY, MT_IMPORT_SIZE_W, pResU, MT_IMPORT_SIZE_W / 2,
						pResV, MT_IMPORT_SIZE_W / 2, MT_IMPORT_SIZE_W, MT_IMPORT_SIZE_H, time_stemp);
					aligned_free(dst);
				}
            } else if (ret <= 0) {
                av_packet_unref(&packet);
                nVideoFlag = true;
            }

		}
		else if (packet.stream_index == m_audio_stream_idx && nAudioFlag == false)
		{
			AVPacket tmpPack;
			tmpPack.data = packet.data;
			tmpPack.size = packet.size;
			while (packet.size > 0)
			{
				ret = avcodec_decode_audio4(m_pAudioCodec, m_pSrcFrame, &frameFinished, &packet);
				packet.size -= ret;
				packet.data += ret;
				if (frameFinished)
				{
					int64_t nPTS = av_frame_get_best_effort_timestamp(m_pSrcFrame);
					double time_stemp = (nPTS - audio_start_pts)* av_q2d(m_pAudioStream->time_base);
					
					if (nPTS < audio_start_pts)
					{
						break;
					}
					if (nPTS > audio_end_pts)
					{
						nAudioFlag = true;
						break;
					}

					recorder.RecordPCM(m_pSrcFrame->extended_data, m_pSrcFrame->nb_samples);
				}
			}
			packet.data = tmpPack.data;
			packet.size = tmpPack.size;
			
		}
        
		av_packet_unref(&packet);
		av_init_packet(&packet);
		if (nAudioFlag && nVideoFlag)
		{
			break;
		}

	}
	av_packet_unref(&packet);

	if (m_ExitAudio == false && !abort_request)
	{
		//≤ª¥Ê‘⁄“Ù∆µ£¨ÃÌº”ø’“Ù∆µ
		int DataSize = (endTime - startTime + 0.1) * 44100*2;
		byte* pData = new byte[DataSize];
		memset(pData, 0, DataSize);
		recorder.RecordPCM(&pData, DataSize/2);
		SAFE_DELETE_ARRAY(pData);
	}

	recorder.Finish();
	recorder.Close();
    
    if (listener) {
        listener->videoEditerProgressEnded(this);
    }

    abort_request = false;
    any_working = false;
    
	SAFE_DELETE_ARRAY(pResYuvData);
	SAFE_DELETE_ARRAY(pFillData);
	return ret;
}


int CVideoEditer::ReCutVideoWithTime(const char* dstFile, double startTime, double endTime)
{
	if (m_pFormatContext == NULL)
	{
		LOGE("No any video is open!");
		return -1;
	}

	double duration = this->GetVideoDuration() + 0.01;

    if (endTime > duration && (endTime - 0.1) < duration) {
        endTime -= 0.1;
    }
    
	if (startTime >= endTime || startTime > duration || endTime > duration)
	{
		LOGE("invalid parameter. start:%f,end:%f,duration:%f", startTime, endTime, duration);
		return -1;
	}

    double cut_total = endTime - startTime + 0.1;
    any_working = true;
    
	CFrameRecorder recorder;
	recorder.Open(dstFile, m_VideoWidth, m_VideoHeight);
	recorder.SetupCropRegion(0, 0, m_VideoWidth, m_VideoHeight, 0);
	if (m_ExitAudio)
	{
		recorder.SetupAudio(m_pAudioCodec->channels, m_pAudioCodec->sample_rate, m_pAudioCodec->sample_fmt);
	}
	else
	{
		recorder.SetupAudio(1, 44100, AV_SAMPLE_FMT_S16);
	}

	recorder.SetupAudioAligen(true);

	recorder.Start();
	double video_pts_base = m_pVideoStream->time_base.num / (double)m_pVideoStream->time_base.den;
	int ret = -1;
	int frameFinished = 0;
	int64_t video_start_pts = (startTime*(m_pVideoStream->time_base.den)) / (m_pVideoStream->time_base.num) + 0.5;
	int64_t video_end_pts = (endTime*(m_pVideoStream->time_base.den)) / (m_pVideoStream->time_base.num) + 0.5;
	int64_t audio_start_pts = 0;
	int64_t audio_end_pts = 0;
	if (m_ExitAudio)
	{
		audio_start_pts = (startTime*(m_pAudioStream->time_base.den)) / (m_pAudioStream->time_base.num) + 0.5;
		audio_end_pts = (endTime*(m_pAudioStream->time_base.den)) / (m_pAudioStream->time_base.num) + 0.5;
	}

	bool nVideoFlag = false;// ”∆µ «∑Ò“—æ≠Ωÿ»°ÕÍ
	bool nAudioFlag = false;//“Ù∆µ «∑Ò“—æ≠Ωÿ»°ÕÍ
	double last_time_stamp = 0.0;
	//---------------seek-------------------------------;
	if (av_seek_frame(m_pFormatContext, m_video_stream_idx, video_start_pts, AVSEEK_FLAG_BACKWARD) < 0)
	{
		LOGE("Failed to seek Video \n");
	}

	if (m_ExitAudio)
	{
		if (av_seek_frame(m_pFormatContext, m_audio_stream_idx, audio_start_pts, AVSEEK_FLAG_BACKWARD) < 0)
		{
			LOGE("Failed to seek Audio \n");
		}
	}
	avcodec_flush_buffers(m_pVideoCodec);
	//------------------end seek----------------------;

    if (listener) {
        listener->videoEditerProgressBegan(this);
    }
    
	AVPacket packet = { 0 };
	av_init_packet(&packet);
	while (av_read_frame(m_pFormatContext, &packet) >= 0 && !abort_request)
	{
		if (packet.stream_index == m_video_stream_idx && nVideoFlag == false)
		{
			ret = avcodec_decode_video2(m_pVideoCodec, m_pSrcFrame, &frameFinished, &packet);
			if (frameFinished)
			{
				int64_t nPTS = av_frame_get_best_effort_timestamp(m_pSrcFrame);

                if(m_CutFrames.size() == 0 && nPTS > video_end_pts)
                {
                    // 解到帧的时候，保证至少有一阵画面。
                    //
                    nPTS = (video_end_pts + video_start_pts)/2;
                    double time_stemp = (nPTS - video_start_pts)* av_q2d(m_pVideoStream->time_base);
                    last_time_stamp = time_stemp;
                    m_pSrcFrame->width = m_VideoWidth;
                    m_pSrcFrame->height = m_VideoHeight;
                    EditorEntity* pEntity = EditorEntityCreate(m_pSrcFrame,time_stemp);
                    m_CutFrames.push(pEntity);
                    continue;
                }

                if (nPTS < video_start_pts)
                {
                    av_packet_unref(&packet);
                    av_init_packet(&packet);
                    continue;
                }
                if (nPTS > video_end_pts)
                {
                    nVideoFlag = true;
                    if (nAudioFlag || m_ExitAudio == false)
                    {
                        break;
                    }
                    av_packet_unref(&packet);
                    av_init_packet(&packet);
                    continue;
                }

                if (nPTS >= video_start_pts &&  nPTS <= video_end_pts)
                {
                    double time_stemp = (nPTS - video_start_pts)* av_q2d(m_pVideoStream->time_base);
                    last_time_stamp = time_stemp;
                    m_pSrcFrame->width = m_VideoWidth;
                    m_pSrcFrame->height = m_VideoHeight;
                    EditorEntity* pEntity = EditorEntityCreate(m_pSrcFrame,time_stemp);
					m_CutFrames.push(pEntity);

				}
			}

		}
		else if (packet.stream_index == m_audio_stream_idx && nAudioFlag == false)
		{
			AVPacket tmpPack;
			tmpPack.data = packet.data;
			tmpPack.size = packet.size;
			while (packet.size > 0)
			{
				ret = avcodec_decode_audio4(m_pAudioCodec, m_pSrcFrame, &frameFinished, &packet);
				packet.size -= ret;
				packet.data += ret;
				if (frameFinished)
				{
					int64_t nPTS = av_frame_get_best_effort_timestamp(m_pSrcFrame);
					double time_stemp = (nPTS - audio_start_pts)* av_q2d(m_pAudioStream->time_base);
					if (nPTS < audio_start_pts)
					{
						break;
					}
					if (nPTS > audio_end_pts)
					{
						nAudioFlag = true;
						break;
					}

					recorder.RecordPCM(m_pSrcFrame->extended_data, m_pSrcFrame->nb_samples);
				}
			}
			packet.data = tmpPack.data;
			packet.size = tmpPack.size;
			
		}
		av_packet_unref(&packet);
		av_init_packet(&packet);
		if (nAudioFlag && nVideoFlag)
		{
			break;
		}

	}
	av_packet_unref(&packet);

	if (m_pVideoCodec && !nVideoFlag && !abort_request)
	{
		for (;!abort_request;)
		{
			av_init_packet(&packet);
			ret = avcodec_decode_video2(m_pVideoCodec, m_pSrcFrame, &frameFinished, &packet);
			double time_stemp = last_time_stamp + 0.033;
			last_time_stamp = time_stemp;
			if (ret < 0)
			{
				av_packet_unref(&packet);
				break;
			}


			if (frameFinished) {
				m_pSrcFrame->width = m_VideoWidth;
				m_pSrcFrame->height = m_VideoHeight;
				EditorEntity* pEntity = EditorEntityCreate(m_pSrcFrame, time_stemp);
				m_CutFrames.push(pEntity);
			}
			else
			{
				av_packet_unref(&packet);
				break;
			}
			av_packet_unref(&packet);
		}
	}
	if (m_ExitAudio == false && !abort_request)
	{
		//≤ª¥Ê‘⁄“Ù∆µ£¨ÃÌº”ø’“Ù∆µ
		int DataSize = (endTime - startTime + 0.1) * 44100 * 2;
		unsigned char* pData = new unsigned char[DataSize];
		memset(pData, 0, DataSize);
		recorder.RecordPCM(&pData, DataSize / 2);
		SAFE_DELETE_ARRAY(pData);
	}
	if (!m_CutFrames.empty())
	{
		EditorEntity* pEntity = m_CutFrames.top();
		double last_pts = pEntity->pts;
		double pts = 0.0;

		while (!m_CutFrames.empty() && !abort_request)
		{
			pEntity = m_CutFrames.top();
			pts += last_pts - pEntity->pts;
			last_pts = pEntity->pts;

			recorder.RecordI420(pEntity->pY, pEntity->stride_y,
				pEntity->pU, pEntity->stride_u,
				pEntity->pV, pEntity->stride_v, pEntity->width, pEntity->height, pts);

			EditorEntityRelease(&pEntity);
			m_CutFrames.pop();
            if (listener) {
                listener->videoEditerProgressChanged(this, pts, cut_total);
            }
		}
	}
	recorder.Finish();
	recorder.Close();
    
    while (!m_CutFrames.empty() && abort_request)
    {
        EditorEntity* pEntity = m_CutFrames.top();
        EditorEntityRelease(&pEntity);
        m_CutFrames.pop();
    }
    
    if (listener) {
        listener->videoEditerProgressEnded(this);
    }
    
    abort_request = false;
    any_working = false;
    
	return ret;

}

/*
int CVideoEditer::CutVideoWithFrame(const char* dstFile, double startTime, double endTime,
	unsigned char r,unsigned char g,unsigned char b)
{
	//À„≥ˆ◊Ó÷’“™ÃÓ≥‰µƒYUV
	unsigned char colorY = RGBToY(r,g,b);//r *  0.299000f + g *  0.587000f + b *  0.114000f;
	unsigned char colorU = RGBToU(r,g,b);;//r * -0.168736f + g * -0.331264f + b *  0.500000f + 128;
	unsigned char colorV = RGBToV(r,g,b);;//r *  0.500000f + g * -.418688f  + b *  -0.081312f + 128;

	if (m_pFormatContext == NULL)
	{
		LOGE("No any video is open!");
		return -1;
	}

	double duration = this->GetVideoDuration() + 0.01;

	if (startTime >= endTime || startTime > duration || endTime > duration)
	{
		LOGE("invalid parameter. start:%f,end:%f,duration:%f", startTime, endTime, duration);
		return -1;
	}

	CFrameRecorder recorder;
	recorder.Open(dstFile, MT_IMPORT_SIZE_W, MT_IMPORT_SIZE_H);
	recorder.SetupCropRegion(0, 0, MT_IMPORT_SIZE_W, MT_IMPORT_SIZE_H, 0);
	if (m_ExitAudio)
	{
		recorder.SetupAudio(m_pAudioCodec->channels, m_pAudioCodec->sample_rate, m_pAudioCodec->sample_fmt);
	}
	else
	{
		recorder.SetupAudio(1, 44100, AV_SAMPLE_FMT_S16);
	}

	recorder.SetupAudioAligen(true);

	recorder.Start();

	double video_pts_base = m_pVideoStream->time_base.num / (double)m_pVideoStream->time_base.den;
	int ret = -1;
	int frameFinished = 0;
	int64_t video_start_pts = (startTime*(m_pVideoStream->time_base.den)) / (m_pVideoStream->time_base.num) + 0.5;
	int64_t video_end_pts = (endTime*(m_pVideoStream->time_base.den)) / (m_pVideoStream->time_base.num) + 0.5;
	int64_t audio_start_pts = 0;
	int64_t audio_end_pts = 0;
	if (m_ExitAudio)
	{
		audio_start_pts = (startTime*(m_pAudioStream->time_base.den)) / (m_pAudioStream->time_base.num) + 0.5;
		audio_end_pts = (endTime*(m_pAudioStream->time_base.den)) / (m_pAudioStream->time_base.num) + 0.5;
	}

	bool nVideoFlag = false;// ”∆µ «∑Ò“—æ≠Ωÿ»°ÕÍ
	bool nAudioFlag = false;//“Ù∆µ «∑Ò“—æ≠Ωÿ»°ÕÍ
	double last_time_stamp = 0.0;
	//---------------seek-------------------------------;
	if (av_seek_frame(m_pFormatContext, m_video_stream_idx, video_start_pts, AVSEEK_FLAG_BACKWARD) < 0)
	{
		LOGE("Failed to seek Video \n");
	}

	if (m_ExitAudio)
	{
		if (av_seek_frame(m_pFormatContext, m_audio_stream_idx, audio_start_pts, AVSEEK_FLAG_BACKWARD) < 0)
		{
			LOGE("Failed to seek Audio \n");
		}
	}
	avcodec_flush_buffers(m_pVideoCodec);
	//------------------end seek----------------------;

	//------------------÷ÿ–¬º∆À„sws--------------------;
	int nDstVideoWidth = 0,nDstVideoHeight = 0;

	if(m_VideoWidth > m_VideoHeight)
	{
		nDstVideoWidth = MT_IMPORT_SIZE_W;
		nDstVideoHeight = (float)nDstVideoWidth/m_VideoWidth * m_VideoHeight;
		//»√≥ˆµƒ∫⁄∞◊ø’º‰“™ƒ‹πª»√4’˚≥˝
		while((MT_IMPORT_SIZE_H - nDstVideoHeight)%4 != 0)
		{
			nDstVideoHeight--;
		}
	}
	else
	{
		nDstVideoHeight = MT_IMPORT_SIZE_H;
		nDstVideoWidth = (float)nDstVideoHeight/m_VideoHeight * m_VideoWidth;
		//»√≥ˆµƒ∫⁄∞◊ø’º‰“™ƒ‹πª»√4’˚≥˝
		while((MT_IMPORT_SIZE_W - nDstVideoWidth)%4 != 0)
		{
			nDstVideoWidth--;
		}
	}

	struct SwsContext * sws_ctx = NULL;
	unsigned char* pFillData = NULL;
	unsigned char* pResYuvData = NULL;
	{
		//need scale
		sws_ctx = sws_getContext(
			m_VideoWidth,
			m_VideoHeight,
			m_pVideoCodec->pix_fmt,
			nDstVideoWidth,
			nDstVideoHeight,
			AV_PIX_FMT_YUV420P,
			SWS_BICUBIC, NULL, NULL, NULL);

		pFillData = new unsigned char[MT_IMPORT_SIZE_W*MT_IMPORT_SIZE_H*3/2];
		pResYuvData = new unsigned char[MT_IMPORT_SIZE_W*MT_IMPORT_SIZE_H*3/2];

	}
	if(pFillData == NULL || pResYuvData == NULL || sws_ctx == NULL)
	{
		LOGE("memory error!");
		return -1;
	}


	//…Í«Î“ª’≈YUVÀı∑≈÷°
	AVFrame*  swsFrame = av_frame_alloc();
	{
		int numBytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, nDstVideoWidth, nDstVideoHeight, 1);
		uint8_t *buffer = (uint8_t *)av_malloc_array(1, numBytes);
        int len = av_image_fill_arrays(swsFrame->data, swsFrame->linesize, buffer, AV_PIX_FMT_YUV420P, nDstVideoWidth, nDstVideoHeight, 1);
	}
	if(swsFrame == NULL)
	{
		LOGE("memory error!");
		return -1;
	}

	//------------------------------------------------;

	AVPacket packet = { 0 };
	av_init_packet(&packet);
	while (av_read_frame(m_pFormatContext, &packet) >= 0)
	{

		if (packet.stream_index == m_video_stream_idx && nVideoFlag == false)
		{
			ret = avcodec_decode_video2(m_pVideoCodec, m_pSrcFrame, &frameFinished, &packet);
			if (frameFinished)
			{
				int64_t nPTS = av_frame_get_best_effort_timestamp(m_pSrcFrame);

				if (nPTS < video_start_pts)
				{
					av_packet_unref(&packet);
					av_init_packet(&packet);
					continue;
				}
				if (nPTS > video_end_pts)
				{
					nVideoFlag = true;
					if (nAudioFlag || m_ExitAudio == false)
					{
						break;
					}
					av_packet_unref(&packet);
					av_init_packet(&packet);
					continue;
				}
				double time_stemp = (nPTS - video_start_pts)* av_q2d(m_pVideoStream->time_base);
				last_time_stamp = time_stemp;

				//–Ë“™◊™≥…YUV420P
				ret = sws_scale(sws_ctx, m_pSrcFrame->data, m_pSrcFrame->linesize, 0, m_pSrcFrame->height, swsFrame->data, swsFrame->linesize);
				if (ret < 0)
				{
					LOGE("error in sws_scale.");
					return -1;
				}
				swsFrame->width = nDstVideoWidth;
				swsFrame->height = nDstVideoHeight;

				FillVideoFrame(pFillData,swsFrame,colorY,colorU,colorV);

				//–˝◊™£ø
				byte* pDstY = pFillData;
				byte* pDstU = pFillData + MT_IMPORT_SIZE_W*MT_IMPORT_SIZE_H;
				byte* pDstV = pDstU + MT_IMPORT_SIZE_W*MT_IMPORT_SIZE_H / 4;

				byte* pResY = pDstY;
				byte* pResU = pDstU;
				byte* pResV = pDstV;
				if (m_Rotate == MT_VIDEO_ROTATE_90)
				{
					pResY = pResYuvData;
					pResU = pResYuvData + MT_IMPORT_SIZE_W*MT_IMPORT_SIZE_H;
					pResV = pResU + MT_IMPORT_SIZE_W*MT_IMPORT_SIZE_H / 4;

					I420Rotate(pDstY, MT_IMPORT_SIZE_W, pDstU, MT_IMPORT_SIZE_W / 2,
						pDstV, MT_IMPORT_SIZE_W / 2, pResY, MT_IMPORT_SIZE_W,
						pResU, MT_IMPORT_SIZE_W / 2, pResV, MT_IMPORT_SIZE_W / 2,
						MT_IMPORT_SIZE_W, MT_IMPORT_SIZE_H, kRotate90);
				}
				else if (m_Rotate == MT_VIDEO_ROTATE_180)
				{
					pResY = pResYuvData;
					pResU = pResYuvData + MT_IMPORT_SIZE_W*MT_IMPORT_SIZE_H;
					pResV = pResU + MT_IMPORT_SIZE_W*MT_IMPORT_SIZE_H / 4;

					I420Rotate(pDstY, MT_IMPORT_SIZE_W, pDstU, MT_IMPORT_SIZE_W / 2,
						pDstV, MT_IMPORT_SIZE_W / 2, pResY, MT_IMPORT_SIZE_W,
						pResU, MT_IMPORT_SIZE_W / 2, pResV, MT_IMPORT_SIZE_W / 2,
						MT_IMPORT_SIZE_W, MT_IMPORT_SIZE_H, kRotate180);
				}
				else if (m_Rotate == MT_VIDEO_ROTATE_270)
				{
					pResY = pResYuvData;
					pResU = pResYuvData + MT_IMPORT_SIZE_W*MT_IMPORT_SIZE_H;
					pResV = pResU + MT_IMPORT_SIZE_W*MT_IMPORT_SIZE_H / 4;

					I420Rotate(pDstY, MT_IMPORT_SIZE_W, pDstU, MT_IMPORT_SIZE_W / 2,
						pDstV, MT_IMPORT_SIZE_W / 2, pResY, MT_IMPORT_SIZE_W,
						pResU, MT_IMPORT_SIZE_W / 2, pResV, MT_IMPORT_SIZE_W / 2,
						MT_IMPORT_SIZE_W, MT_IMPORT_SIZE_H, kRotate270);
				}

				recorder.RecordI420(pResY, MT_IMPORT_SIZE_W, pResU, MT_IMPORT_SIZE_W / 2,
					pResV, MT_IMPORT_SIZE_W / 2, MT_IMPORT_SIZE_W, MT_IMPORT_SIZE_H, time_stemp);
			}

		}
		else if (packet.stream_index == m_audio_stream_idx && nAudioFlag == false)
		{
			AVPacket tmpPack;
			tmpPack.data = packet.data;
			tmpPack.size = packet.size;
			while (packet.size > 0)
			{
				ret = avcodec_decode_audio4(m_pAudioCodec, m_pSrcFrame, &frameFinished, &packet);
				packet.size -= ret;
				packet.data += ret;
				if (frameFinished)
				{
					int64_t nPTS = av_frame_get_best_effort_timestamp(m_pSrcFrame);
					double time_stemp = (nPTS - audio_start_pts)* av_q2d(m_pAudioStream->time_base);

					if (nPTS < audio_start_pts)
					{
						break;
					}
					if (nPTS > audio_end_pts)
					{
						nAudioFlag = true;
						break;
					}

					recorder.RecordPCM(m_pSrcFrame->extended_data, m_pSrcFrame->nb_samples);
					//ar.RecordPCM(m_pSrcFrame->extended_data, m_pSrcFrame->nb_samples);
				}
			}
			packet.data = tmpPack.data;
			packet.size = tmpPack.size;

		}
		av_packet_unref(&packet);
		av_init_packet(&packet);
		if (nAudioFlag && nVideoFlag)
		{
			break;
		}

	}
	av_packet_unref(&packet);


	if (m_pVideoCodec && !nVideoFlag)
	{
		for (;;)
		{
			av_init_packet(&packet);
			ret = avcodec_decode_video2(m_pVideoCodec, m_pSrcFrame, &frameFinished, &packet);
			double time_stemp = last_time_stamp + 0.033;
			last_time_stamp = time_stemp;
			if (ret < 0)
			{
				av_packet_unref(&packet);
				break;
			}


			if (frameFinished)
			{
				int64_t nPTS = av_frame_get_best_effort_timestamp(m_pSrcFrame);

				if (nPTS < video_start_pts)
				{
					av_packet_unref(&packet);
					av_init_packet(&packet);
					continue;
				}
				if (nPTS > video_end_pts)
				{
					nVideoFlag = true;
					if (nAudioFlag || m_ExitAudio == false)
					{
						break;
					}
					av_packet_unref(&packet);
					av_init_packet(&packet);
					continue;
				}
				double time_stemp = (nPTS - video_start_pts)* av_q2d(m_pVideoStream->time_base);
				last_time_stamp = time_stemp;

				//–Ë“™◊™≥…YUV420P
				ret = sws_scale(sws_ctx, m_pSrcFrame->data, m_pSrcFrame->linesize, 0, m_pSrcFrame->height, swsFrame->data, swsFrame->linesize);
				if (ret < 0)
				{
					LOGE("error in sws_scale.");
					return -1;
				}
				swsFrame->width = nDstVideoWidth;
				swsFrame->height = nDstVideoHeight;

				FillVideoFrame(pFillData,swsFrame,colorY,colorU,colorV);

				//–˝◊™£ø
				byte* pDstY = pFillData;
				byte* pDstU = pFillData + MT_IMPORT_SIZE_W*MT_IMPORT_SIZE_H;
				byte* pDstV = pDstU + MT_IMPORT_SIZE_W*MT_IMPORT_SIZE_H / 4;

				byte* pResY = pDstY;
				byte* pResU = pDstU;
				byte* pResV = pDstV;
				if (m_Rotate == MT_VIDEO_ROTATE_90)
				{
					pResY = pResYuvData;
					pResU = pResYuvData + MT_IMPORT_SIZE_W*MT_IMPORT_SIZE_H;
					pResV = pResU + MT_IMPORT_SIZE_W*MT_IMPORT_SIZE_H / 4;

					I420Rotate(pDstY, MT_IMPORT_SIZE_W, pDstU, MT_IMPORT_SIZE_W / 2,
						pDstV, MT_IMPORT_SIZE_W / 2, pResY, MT_IMPORT_SIZE_W,
						pResU, MT_IMPORT_SIZE_W / 2, pResV, MT_IMPORT_SIZE_W / 2,
						MT_IMPORT_SIZE_W, MT_IMPORT_SIZE_H, kRotate90);
				}
				else if (m_Rotate == MT_VIDEO_ROTATE_180)
				{
					pResY = pResYuvData;
					pResU = pResYuvData + MT_IMPORT_SIZE_W*MT_IMPORT_SIZE_H;
					pResV = pResU + MT_IMPORT_SIZE_W*MT_IMPORT_SIZE_H / 4;

					I420Rotate(pDstY, MT_IMPORT_SIZE_W, pDstU, MT_IMPORT_SIZE_W / 2,
						pDstV, MT_IMPORT_SIZE_W / 2, pResY, MT_IMPORT_SIZE_W,
						pResU, MT_IMPORT_SIZE_W / 2, pResV, MT_IMPORT_SIZE_W / 2,
						MT_IMPORT_SIZE_W, MT_IMPORT_SIZE_H, kRotate180);
				}
				else if (m_Rotate == MT_VIDEO_ROTATE_270)
				{
					pResY = pResYuvData;
					pResU = pResYuvData + MT_IMPORT_SIZE_W*MT_IMPORT_SIZE_H;
					pResV = pResU + MT_IMPORT_SIZE_W*MT_IMPORT_SIZE_H / 4;

					I420Rotate(pDstY, MT_IMPORT_SIZE_W, pDstU, MT_IMPORT_SIZE_W / 2,
						pDstV, MT_IMPORT_SIZE_W / 2, pResY, MT_IMPORT_SIZE_W,
						pResU, MT_IMPORT_SIZE_W / 2, pResV, MT_IMPORT_SIZE_W / 2,
						MT_IMPORT_SIZE_W, MT_IMPORT_SIZE_H, kRotate270);
				}

				recorder.RecordI420(pResY, MT_IMPORT_SIZE_W, pResU, MT_IMPORT_SIZE_W / 2,
					pResV, MT_IMPORT_SIZE_W / 2, MT_IMPORT_SIZE_W, MT_IMPORT_SIZE_H, time_stemp);
			}
			else
			{
				av_packet_unref(&packet);
				break;
			}
			av_packet_unref(&packet);
		}
	}

	if (sws_ctx)
	{
		sws_freeContext(sws_ctx);
		sws_ctx = NULL;
	}
	if (swsFrame)
	{
		if (swsFrame->data[0])
		{
			av_free(swsFrame->data[0]);
			swsFrame->data[0] = NULL;
		}
		av_frame_free(&swsFrame);
		swsFrame = NULL;
	}

	SAFE_DELETE_ARRAY(pFillData);
	SAFE_DELETE_ARRAY(pResYuvData);

	if (m_ExitAudio == false)
	{
		//≤ª¥Ê‘⁄“Ù∆µ£¨ÃÌº”ø’“Ù∆µ
		int DataSize = (endTime - startTime + 0.1) * 44100 * 2;
		byte* pData = new byte[DataSize];
		memset(pData, 0, DataSize);
		recorder.RecordPCM(&pData, DataSize / 2);
		SAFE_DELETE_ARRAY(pData);
	}

	recorder.Finish();
	recorder.Close();
	return ret;
}
*/

void CVideoEditer::Close()
{
	while (!m_CutFrames.empty())
	{
		EditorEntity* entity = m_CutFrames.top();
		EditorEntityRelease(&entity);
		m_CutFrames.pop();

	}

	if (m_pVideoStream && m_pVideoStream->codec)
	{
		avcodec_close(m_pVideoStream->codec);
		m_pVideoCodec = NULL;
		m_pVideoStream = NULL;
	}

	if (m_pAudioStream && m_pAudioStream->codec)
	{
		avcodec_close(m_pAudioStream->codec);
		m_pAudioCodec = NULL;
		m_pAudioStream = NULL;
	}

	if (m_pFormatContext)
	{
		avformat_close_input(&m_pFormatContext);
		m_pFormatContext = NULL;
	}
	if (m_pSrcFrame)
	{
		av_frame_free(&m_pSrcFrame);
		m_pSrcFrame = NULL;
	}
	if (m_pRGBFrame)
	{
		for (int i = 0; i < AV_NUM_DATA_POINTERS; ++i)
		{
			uint8_t * pDataMem = m_pRGBFrame->data[i];
			if (pDataMem != NULL)
			{
				av_free(pDataMem);
				m_pRGBFrame->data[i] = NULL;
			}
		}
		av_frame_free(&m_pRGBFrame);
		m_pRGBFrame = NULL;
	}

	if (rgb_sws_ctx)
	{
		sws_freeContext(rgb_sws_ctx);
		rgb_sws_ctx = NULL;
	}

	if (yuv_sws_ctx)
	{
		sws_freeContext(yuv_sws_ctx);
		yuv_sws_ctx = NULL;
	}

	if (m_YuvFrame)
	{
		if (m_YuvFrame->data[0])
		{
			av_free(m_YuvFrame->data[0]);
			m_YuvFrame->data[0] = NULL;
		}
		av_frame_free(&m_YuvFrame);
		m_YuvFrame = NULL;
	}
	m_DstVideoWidth = -1;
	m_DstVideoHeight = -1;
	m_ExitAudio = false;
	m_Left = 0;
	m_Top = 0;
	m_KeyFrameWidth = 240;
	m_KeyFrameHeight = 240;
	m_video_stream_idx = -1;
	m_audio_stream_idx = -1;
	m_KeyFrameStep = 2.0;
	m_Rotate = MT_VIDEO_ROTATE_0;
}

int CVideoEditer::OtoMad( const char* dstFile,double startTime,double endTime,int times,bool isReverse /*= false*/ )
{
	if (m_pFormatContext == NULL)
	{
		LOGE("No any video is open!");
		return -1;
	}

	double duration = this->GetVideoDuration();

	if (startTime >= endTime || startTime > duration || endTime > duration)
	{
		LOGE("invalid parameter. start:%f,end:%f,duration:%f", startTime, endTime, duration);
		return -1;
	}

	CFrameRecorder recorder;
	recorder.Open(dstFile, m_VideoWidth, m_VideoHeight);
	recorder.SetupCropRegion(0, 0, m_VideoWidth, m_VideoHeight, 0);
	if (m_ExitAudio)
	{
		recorder.SetupAudio(m_pAudioCodec->channels, m_pAudioCodec->sample_rate, m_pAudioCodec->sample_fmt);
	}
	else
	{
		recorder.SetupAudio(1, 44100, AV_SAMPLE_FMT_S16);
	}
	recorder.SetupAudioAligen(true);

	recorder.Start();

	int ret = -1;
	int frameFinished = 0;
	int64_t video_start_pts = (startTime*(m_pVideoStream->time_base.den)) / (m_pVideoStream->time_base.num) + 0.5;
	int64_t video_end_pts = (endTime*(m_pVideoStream->time_base.den)) / (m_pVideoStream->time_base.num) + 0.5;
	int64_t audio_start_pts = 0;
	int64_t audio_end_pts = 0;
	if (m_ExitAudio)
	{
		audio_start_pts = (startTime*(m_pAudioStream->time_base.den)) / (m_pAudioStream->time_base.num) + 0.5;
		audio_end_pts = (endTime*(m_pAudioStream->time_base.den)) / (m_pAudioStream->time_base.num) + 0.5;
	}

	bool nVideoFlag = false;//视频是否已经截取完
	bool nAudioFlag = false;//音频是否已经截取完
	double last_time_stamp = 0.0;
	//---------------seek-------------------------------;
	if (av_seek_frame(m_pFormatContext, m_video_stream_idx, video_start_pts, AVSEEK_FLAG_BACKWARD) < 0)
	{
		LOGE("Failed to seek Video \n");
	}

	if (m_ExitAudio)
	{
		if (av_seek_frame(m_pFormatContext, m_audio_stream_idx, audio_start_pts, AVSEEK_FLAG_BACKWARD) < 0)
		{
			LOGE("Failed to seek Audio \n");
		}
	}
	avcodec_flush_buffers(m_pVideoCodec);
	//------------------end seek----------------------;
	//申请一个帧缓冲区
	vector<EditorEntity*> vecVideoFrames;
	vector<AudioEntity*> vecAudioFrames;
	//////////////////////////////////////////////////////////////////////////
	EditorEntity* pZeroEntity = NULL;
	AVPacket packet = { 0 };
	av_init_packet(&packet);
	while (av_read_frame(m_pFormatContext, &packet) >= 0)
	{

		if (packet.stream_index == m_video_stream_idx && nVideoFlag == false)
		{
			ret = avcodec_decode_video2(m_pVideoCodec, m_pSrcFrame, &frameFinished, &packet);
			if (frameFinished)
			{
				int64_t nPTS = av_frame_get_best_effort_timestamp(m_pSrcFrame);
				if(pZeroEntity == NULL)
				{
					pZeroEntity = EditorEntityCreate(m_pSrcFrame,0);
				}
				if (nPTS < video_start_pts)
				{
					av_packet_unref(&packet);
					av_init_packet(&packet);
					continue;
				}
				if (nPTS > video_end_pts)
				{
					nVideoFlag = true;
					if (nAudioFlag || m_ExitAudio == false)
					{
						break;
					}
					av_packet_unref(&packet);
					av_init_packet(&packet);
					continue;
				}

				if (nPTS >= video_start_pts &&  nPTS <= video_end_pts)
				{
					double time_stemp = (nPTS - video_start_pts)* av_q2d(m_pVideoStream->time_base);
					last_time_stamp = time_stemp;
					EditorEntity* pEntity = EditorEntityCreate(m_pSrcFrame,time_stemp);
					vecVideoFrames.push_back(pEntity);

				}
			}

		}
		else if (packet.stream_index == m_audio_stream_idx && nAudioFlag == false)
		{
			AVPacket tmpPack;
			tmpPack.data = packet.data;
			tmpPack.size = packet.size;
			while (packet.size > 0)
			{
				ret = avcodec_decode_audio4(m_pAudioCodec, m_pSrcFrame, &frameFinished, &packet);
				packet.size -= ret;
				packet.data += ret;
				if (frameFinished)
				{
					int64_t nPTS = av_frame_get_best_effort_timestamp(m_pSrcFrame);
					double time_stemp = (nPTS - audio_start_pts)* av_q2d(m_pAudioStream->time_base);
					if (nPTS < audio_start_pts)
					{
						break;
					}
					if (nPTS > audio_end_pts)
					{
						nAudioFlag = true;
						break;
					}
					if (m_pSrcFrame->best_effort_timestamp >= audio_start_pts && m_pSrcFrame->best_effort_timestamp <= audio_end_pts)
					{
						AudioEntity* pEntity = AudioEntityCreate(m_pSrcFrame,time_stemp);
						vecAudioFrames.push_back(pEntity);
						//recorder.RecordPCM(m_pSrcFrame->extended_data, m_pSrcFrame->nb_samples);
					}

				}
			}
			packet.data = tmpPack.data;
			packet.size = tmpPack.size;

		}
		av_packet_unref(&packet);
		av_init_packet(&packet);
		if (nAudioFlag && nVideoFlag)
		{
			break;
		}

	}
	av_packet_unref(&packet);


	if (m_pVideoCodec && !nVideoFlag)
	{
		for (;;)
		{
			av_init_packet(&packet);
			ret = avcodec_decode_video2(m_pVideoCodec, m_pSrcFrame, &frameFinished, &packet);
			double time_stemp = last_time_stamp + 0.033;
			last_time_stamp = time_stemp;
			if (ret < 0)
			{
				av_packet_unref(&packet);
				break;
			}


			if (frameFinished) {

				EditorEntity* pEntity = EditorEntityCreate(m_pSrcFrame, time_stemp);
				vecVideoFrames.push_back(pEntity);
			}
			else
			{
				av_packet_unref(&packet);
				break;
			}
			av_packet_unref(&packet);
		}
	}

	if (m_ExitAudio == false)
	{
		//不存在音频，添加空音频
		int DataSize = (endTime - startTime + 0.1) * 44100 * 2;
		DataSize*= times;
		if(isReverse)
		{
			DataSize*=2;
		}
		unsigned char* pData = new unsigned char[DataSize];
		memset(pData, 0, DataSize);
		recorder.RecordPCM(&pData, DataSize / 2);
		SAFE_DELETE_ARRAY(pData);
	}
	else
	{
		int audioTimes = times;
		if(isReverse) audioTimes*=2;
		for(int k = 0 ; k < audioTimes ; k ++)
		{
			for(int i = 0 ; i < vecAudioFrames.size();i++)
			{
				recorder.RecordPCM(vecAudioFrames[i]->data,vecAudioFrames[i]->nb_samples);
			}
		}
	}
	double startPts = 0.0f;
	if (!vecVideoFrames.empty())
	{
		startPts = vecVideoFrames[0]->pts;
		double decodeDuration = vecVideoFrames[vecVideoFrames.size()-1]->pts - vecVideoFrames[0]->pts;
		for(int k = 0 ; k < times ; k ++)
		{
			for(int i = 0 ; i < vecVideoFrames.size();i++)
			{
				EditorEntity* pEntity = vecVideoFrames[i];
				double addPts = 0.0f;
				if(i!= 0)
				{
					addPts =  vecVideoFrames[i]->pts - vecVideoFrames[i-1]->pts;
				}
				double pts = startPts + addPts;
				recorder.RecordI420(pEntity->pY, pEntity->stride_y,
					pEntity->pU, pEntity->stride_u,
					pEntity->pV, pEntity->stride_v, pEntity->width, pEntity->height, pts);
				startPts += addPts;
			}

			if(endTime - startTime > decodeDuration)
			{
				startPts += (endTime-startTime)-decodeDuration;
			}

			if(isReverse)
			{
				for(int i = vecVideoFrames.size() - 1 ; i >= 0 ; i --)
				{
					EditorEntity* pEntity = vecVideoFrames[i];
					double addPts = 0.0f;
					if(i != vecVideoFrames.size()-1)
					{
						addPts = vecVideoFrames[i+1]->pts - vecVideoFrames[i]->pts;
					}
					double pts = startPts + addPts;

					recorder.RecordI420(pEntity->pY, pEntity->stride_y,
						pEntity->pU, pEntity->stride_u,
						pEntity->pV, pEntity->stride_v, pEntity->width, pEntity->height, pts);
					startPts += addPts;
				}
				if(endTime - startTime > decodeDuration)
				{
					startPts += (endTime-startTime)-decodeDuration;
				}
			}
		}
	}
	if(vecVideoFrames.size())
	{
		EditorEntity* pEntity = vecVideoFrames[vecVideoFrames.size()-1];
		if(isReverse)
		{
			pEntity = vecVideoFrames[0];
		}
		recorder.RecordI420(pEntity->pY, pEntity->stride_y,
			pEntity->pU, pEntity->stride_u,
			pEntity->pV, pEntity->stride_v, pEntity->width, pEntity->height, startPts);
	}

	if(vecVideoFrames.size() == 0 && pZeroEntity)
	{
		//没有帧，补2帧;
		startPts = 0.0;
		const int nAddplus = 2;
		for(int i = 0 ; i < nAddplus ; i ++)
		{
			recorder.RecordI420(pZeroEntity->pY, pZeroEntity->stride_y,
				pZeroEntity->pU, pZeroEntity->stride_u,
				pZeroEntity->pV, pZeroEntity->stride_v, pZeroEntity->width, pZeroEntity->height, i*0.033333f);

		}
		startPts = (endTime - startTime)*times;
		if(isReverse)
		{
			startPts*=2.0;
		}
		if(startPts - nAddplus*0.03333f > 0.01f)
		{
			recorder.RecordI420(pZeroEntity->pY, pZeroEntity->stride_y,
				pZeroEntity->pU, pZeroEntity->stride_u,
				pZeroEntity->pV, pZeroEntity->stride_v, pZeroEntity->width, pZeroEntity->height, startPts);
		}
	}
    
    if(pZeroEntity)
    {
        EditorEntityRelease(&pZeroEntity);
    }

	//release;

	for(int i = 0 ; i < vecVideoFrames.size();i++)
	{
		EditorEntity* pEntity = vecVideoFrames[i];
		EditorEntityRelease(&pEntity);
	}
	vecVideoFrames.clear();
	for(int i = 0 ; i < vecAudioFrames.size();i++)
	{
		AudioEntity* pEntity = vecAudioFrames[i];
		AudioEntityRelease(&pEntity);
	}
	vecAudioFrames.clear();

	recorder.Finish();
	recorder.Close();
	return ret;


}

int CVideoEditer::OtoMad( const char* dstFile,vector<TimeNode>& timeNodes )
{
	if (m_pFormatContext == NULL || timeNodes.size() == 0)
	{
		LOGE("No any video is open!");
		return -1;
	}

	double duration = this->GetVideoDuration();
	double startTime = duration;
	double endTime = 0.0;
	double resultTime = 0.0f;
	for(int i = 0 ; i < timeNodes.size() ; i ++)
	{
		if(timeNodes[i].end - timeNodes[i].start < 0.01)
		{
			LOGE("warning : timeNode is too close.(%.4lf,%.4lf)",timeNodes[i].start,timeNodes[i].end);
		}

		if(timeNodes[i].end > duration)
		{
			LOGE("warning: timeNode is bigger then duration.(%.4lf,%.4lf)",timeNodes[i].start,timeNodes[i].end);
		}

		if(timeNodes[i].start < startTime)
		{
			startTime = timeNodes[i].start;
		}
		if(timeNodes[i].end > endTime)
		{
			endTime = timeNodes[i].end;
		}
		resultTime += (timeNodes[i].end - timeNodes[i].start);
	}
	endTime += 0.06;
	LOGE("result time = %.2f\n",resultTime);
	for(int i = 0 ; i < timeNodes.size(); i ++)
	{
		timeNodes[i].end -= startTime;
		timeNodes[i].start -= startTime;
	}

	CFrameRecorder recorder;
	recorder.Open(dstFile, m_VideoWidth, m_VideoHeight);
	recorder.SetupCropRegion(0, 0, m_VideoWidth, m_VideoHeight, 0);
	if (m_ExitAudio)
	{
		recorder.SetupAudio(m_pAudioCodec->channels, m_pAudioCodec->sample_rate, m_pAudioCodec->sample_fmt);
	}
	else
	{
		recorder.SetupAudio(1, 44100, AV_SAMPLE_FMT_S16);
	}
	recorder.SetupAudioAligen(true);

	recorder.Start();
	double video_pts_base = m_pVideoStream->time_base.num / (double)m_pVideoStream->time_base.den;
	int ret = -1;
	int frameFinished = 0;
	int64_t video_start_pts = (startTime*(m_pVideoStream->time_base.den)) / (m_pVideoStream->time_base.num) + 0.5;
	int64_t video_end_pts = (endTime*(m_pVideoStream->time_base.den)) / (m_pVideoStream->time_base.num) + 0.5;
	int64_t audio_start_pts = 0;
	int64_t audio_end_pts = 0;
	if (m_ExitAudio)
	{
		audio_start_pts = (startTime*(m_pAudioStream->time_base.den)) / (m_pAudioStream->time_base.num) + 0.5;
		audio_end_pts = (endTime*(m_pAudioStream->time_base.den)) / (m_pAudioStream->time_base.num) + 0.5;
	}

	bool nVideoFlag = false;//视频是否已经截取完
	bool nAudioFlag = false;//音频是否已经截取完
	double last_time_stamp = 0.0;
	//---------------seek-------------------------------;
	if (av_seek_frame(m_pFormatContext, m_video_stream_idx, video_start_pts, AVSEEK_FLAG_BACKWARD) < 0)
	{
		LOGE("Failed to seek Video \n");
	}

	if (m_ExitAudio)
	{
		if (av_seek_frame(m_pFormatContext, m_audio_stream_idx, audio_start_pts, AVSEEK_FLAG_BACKWARD) < 0)
		{
			LOGE("Failed to seek Audio \n");
		}
	}
	avcodec_flush_buffers(m_pVideoCodec);
	//------------------end seek----------------------;
	//申请一个帧缓冲区
	vector<EditorEntity*> vecVideoFrames;
	vector<AudioEntity*> vecAudioFrames;
	//////////////////////////////////////////////////////////////////////////
	EditorEntity* pZeroEntity = NULL;
	AVPacket packet = { 0 };
	av_init_packet(&packet);
	while (av_read_frame(m_pFormatContext, &packet) >= 0)
	{

		if (packet.stream_index == m_video_stream_idx && nVideoFlag == false)
		{
			ret = avcodec_decode_video2(m_pVideoCodec, m_pSrcFrame, &frameFinished, &packet);
			if (frameFinished)
			{
				int64_t nPTS = av_frame_get_best_effort_timestamp(m_pSrcFrame);
				if(pZeroEntity == NULL)
				{
					pZeroEntity = EditorEntityCreate(m_pSrcFrame,0);
				}
				if (nPTS < video_start_pts)
				{
					av_packet_unref(&packet);
					av_init_packet(&packet);
					continue;
				}
				if (nPTS > video_end_pts)
				{
					nVideoFlag = true;
					if (nAudioFlag || m_ExitAudio == false)
					{
						break;
					}
					av_packet_unref(&packet);
					av_init_packet(&packet);
					continue;
				}

				if (nPTS >= video_start_pts &&  nPTS <= video_end_pts)
				{
					double time_stemp = (nPTS - video_start_pts)* av_q2d(m_pVideoStream->time_base);
					last_time_stamp = time_stemp;
					EditorEntity* pEntity = EditorEntityCreate(m_pSrcFrame,time_stemp);
					vecVideoFrames.push_back(pEntity);

				}
			}

		}
		else if (packet.stream_index == m_audio_stream_idx && nAudioFlag == false)
		{
			AVPacket tmpPack;
			tmpPack.data = packet.data;
			tmpPack.size = packet.size;
			while (packet.size > 0)
			{
				ret = avcodec_decode_audio4(m_pAudioCodec, m_pSrcFrame, &frameFinished, &packet);
				packet.size -= ret;
				packet.data += ret;
				if (frameFinished)
				{
					int64_t nPTS = av_frame_get_best_effort_timestamp(m_pSrcFrame);
					double time_stemp = (nPTS - audio_start_pts)* av_q2d(m_pAudioStream->time_base);
					if (nPTS < audio_start_pts)
					{
						break;
					}
					if (nPTS > audio_end_pts)
					{
						nAudioFlag = true;
						break;
					}
					if (m_pSrcFrame->best_effort_timestamp >= audio_start_pts && m_pSrcFrame->best_effort_timestamp <= audio_end_pts)
					{
						AudioEntity* pEntity = AudioEntityCreate(m_pSrcFrame,time_stemp);
						vecAudioFrames.push_back(pEntity);
						//recorder.RecordPCM(m_pSrcFrame->extended_data, m_pSrcFrame->nb_samples);
					}

				}
			}
			packet.data = tmpPack.data;
			packet.size = tmpPack.size;

		}
		av_packet_unref(&packet);
		av_init_packet(&packet);
		if (nAudioFlag && nVideoFlag)
		{
			break;
		}

	}
	av_packet_unref(&packet);


	if (m_pVideoCodec && !nVideoFlag)
	{
		for (;;)
		{
			av_init_packet(&packet);
			ret = avcodec_decode_video2(m_pVideoCodec, m_pSrcFrame, &frameFinished, &packet);
			double time_stemp = last_time_stamp + 0.033;
			last_time_stamp = time_stemp;
			if (ret < 0)
			{
				av_packet_unref(&packet);
				break;
			}


			if (frameFinished) {

				EditorEntity* pEntity = EditorEntityCreate(m_pSrcFrame, time_stemp);
				vecVideoFrames.push_back(pEntity);
			}
			else
			{
				av_packet_unref(&packet);
				break;
			}
			av_packet_unref(&packet);
		}
	}

	double startPts = 0.0f;
	EditorEntity* lastEntity = NULL;
	if(vecVideoFrames.size() != 0)
	{
		startPts = vecVideoFrames[0]->pts;

		for(int i = 0 ; i < timeNodes.size() ; i ++)
		{
			//VIDEO;
			int start = SearchPosition(vecVideoFrames,timeNodes[i].start);
			int end = SearchPosition(vecVideoFrames,timeNodes[i].end);
			double addDuration = startPts;
			for(int j = start ; j <= end ; j ++)
			{
				EditorEntity* pEntity = vecVideoFrames[j];

				recorder.RecordI420(pEntity->pY, pEntity->stride_y,
					pEntity->pU, pEntity->stride_u,
					pEntity->pV, pEntity->stride_v, pEntity->width, pEntity->height, startPts);

				if(j!= end)
				{
					startPts += vecVideoFrames[j + 1]->pts - vecVideoFrames[j]->pts;
				}
				else
				{
					startPts += 0.033333;
					addDuration = startPts - addDuration;
					if(addDuration < timeNodes[i].end - timeNodes[i].start)
					{
						startPts += (timeNodes[i].end - timeNodes[i].start) - addDuration;
					}
				}
				lastEntity = pEntity;
			}
			if(m_ExitAudio)
			{
				int start = SearchPosition(vecAudioFrames,timeNodes[i].start);
				int end = SearchPosition(vecAudioFrames,timeNodes[i].end);


				for(int j = start ; j <= end ; j ++)
				{
					recorder.RecordPCM(vecAudioFrames[j]->data,vecAudioFrames[j]->nb_samples);
				}
			}
		}
	}

	if(vecVideoFrames.size()&& lastEntity)
	{
		EditorEntity* pEntity = lastEntity;
		recorder.RecordI420(pEntity->pY, pEntity->stride_y,
			pEntity->pU, pEntity->stride_u,
			pEntity->pV, pEntity->stride_v, pEntity->width, pEntity->height, startPts);
	}

	if (m_ExitAudio == false)
	{
		//不存在音频，添加空音频;
		int DataSize = (resultTime) * 44100 * 2;
		unsigned char* pData = new unsigned char[DataSize];
		memset(pData, 0, DataSize);
		recorder.RecordPCM(&pData, DataSize / 2);
		SAFE_DELETE_ARRAY(pData);
	}
	if(vecVideoFrames.size() == 0 && pZeroEntity)
	{
		//没有帧，补2帧;
		startPts = 0.0;
		const int nAddplus = 2;
		double startPts = 0.0f;
		for(int i = 0 ; i < timeNodes.size() ; i ++)
		{
			startPts += timeNodes[i].end - timeNodes[i].start;
		}
		for(int i = 0 ; i < nAddplus ; i ++)
		{
			recorder.RecordI420(pZeroEntity->pY, pZeroEntity->stride_y,
				pZeroEntity->pU, pZeroEntity->stride_u,
				pZeroEntity->pV, pZeroEntity->stride_v, pZeroEntity->width, pZeroEntity->height, i*0.033333f);

		}
		if(startPts - nAddplus*0.03333f > 0.01f)
		{
			recorder.RecordI420(pZeroEntity->pY, pZeroEntity->stride_y,
				pZeroEntity->pU, pZeroEntity->stride_u,
				pZeroEntity->pV, pZeroEntity->stride_v, pZeroEntity->width, pZeroEntity->height, startPts);
		}
	}
	if(pZeroEntity)
	{
		EditorEntityRelease(&pZeroEntity);
	}


	for(int i = 0 ; i < vecVideoFrames.size();i++)
	{
		EditorEntity* pEntity = vecVideoFrames[i];
		EditorEntityRelease(&pEntity);
	}
	vecVideoFrames.clear();
	for(int i = 0 ; i < vecAudioFrames.size();i++)
	{
		AudioEntity* pEntity = vecAudioFrames[i];
		AudioEntityRelease(&pEntity);
	}
	vecAudioFrames.clear();

	recorder.Finish();
	recorder.Close();
	return ret;

}
