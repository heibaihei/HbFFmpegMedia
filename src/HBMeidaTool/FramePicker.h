#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#ifdef __cplusplus
}
#endif


class CFramePicker
{
public:
	CFramePicker();
	~CFramePicker();
private:
	AVFormatContext* m_pFormatContext;
	AVStream *m_pVideoStream;
	AVCodecContext *m_pVideoCodec;
	AVFrame* m_pSrcFrame;
	AVFrame* m_pRGBFrame;
	struct SwsContext* sws_ctx;
	int m_video_stream_idx;

	int m_VideoWidth;
	int m_VideoHeight;
	double m_Duration;
    int nailWidth;
    int nailHeight;
    uint8_t *rgbBuf = NULL;
public:
    int (*m_SaveFunc)(unsigned char* pARGB, int width, int height, int iFrame);
    int (*notifyProgess)(void *obj, int frameNum);

	int Open(const char* file, bool isThumbnail);
	void Close();	
	double GetVideoDuration();
    /**
     *  fan'hui
     *
     *  @param stemps 时间节点数组
     *  @param length 长度
     *
     *  @return 返回成功的张数
     */
	int GetKeyFrameOrder(double* stemps, int length, void *obj);

};

