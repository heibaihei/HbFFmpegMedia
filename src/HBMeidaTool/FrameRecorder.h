#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/audio_fifo.h>
#ifdef __cplusplus
}
#endif
#include <pthread.h>

#include "rotate_yuv.h"
#include "convert_yuv.h"
#include "ADD_WaterMark.h"

typedef enum MediaRecorderQuality{
	//1.2.0 固定质量
	MEDIARECORDER_QINQMAX,
	//1.2.2 固定码率
	MEDIARECORDER_BITRATE
}MediaRecorderQuality;

typedef enum{
	FR_FILL_NONE,
	FR_FILL_BLACK,
	FR_FILL_WHITE
}EncodeFillMode;

#define IFRAME_INTERNAL_AUTO -1

class CFrameRecorder
{
private:
    // add by Javan
    unsigned char *last_frame_data;
    int last_frame_w, last_frame_h;
    double last_frame_timesample;
    // end of add.

	AVFormatContext *m_pOutputFormatContext;
	AVOutputFormat *m_pOutputFormat;
	AVStream *m_pDstVideoStream;
	AVCodecContext *m_oVcc;
	AVCodec *m_pDstVideoCodec;
	AVFrame* m_pVideoFrame;
	struct SwsContext *sws_ctx;

	char m_FilePath[300];

	AVStream* m_pDstAudioStream ;
	AVCodecContext *m_oAcc;
	AVCodec *m_pDstAudioCodec;
	AVFrame *m_pAudioFrame;
	AVAudioFifo * m_avFIFO;
	struct SwrContext *swr_ctx;
	uint8_t **dst_samples_data;
	int	dst_samples_linesize;
	int src_nb_samples;
	int dst_samples_size;
	int max_dst_nb_samples;
	bool m_AudioAligen;
	double m_MaxVideoTimeStamp;
	double m_MaxAudioTimeStamp;
	int m_FrameRate ;
	int m_CropLeft;
	int m_CropTop;
	int m_CropRight;
	int m_CropBottom;
	int m_CropWidth;
	int m_CropHeight;
	RotationMode m_rotateMode;
	MediaRecorderQuality m_Quality;
	int m_LastFrameIndex;	//上一次编码的帧索引号
	unsigned char* CropI420(unsigned char* pY, int stride_y, unsigned char* pU, int stride_u,
								unsigned char* pV, int stride_v, int width, int height);

	unsigned char* CropYuv420sp(unsigned char* src, int width, int height);
	//填充指定颜色
	void FillTopBottomColor(unsigned char* pI420,int width,int height);

    int32_t audio_pcm_length;
	//ԭʼsamplae_rate
    int src_sample_rate;
	int m_RecordChannle;
	//填充模式
	EncodeFillMode m_FillMode;
	int m_FillPixels;
	//是否限制最大帧率为30
	bool m_IsMaxFrameRate30;
	//录制的最近一帧的时间戳
	double m_LastTimeStamp;
	//帧的时间戳间隔;
	double m_FrameInterval;
	//导入视频水印相关
	//is watermark
	bool m_IsUseWaterMark;
	//水印类
	ADD_WaterMark m_WaterMark;
	//水印类型
	MeipaiWatermarkType m_WatermarkType;
	//
	double m_VideoDuration; //要编写的视频总时长
	ADD_WaterMark m_EndingWatermark;
	MeipaiWatermarkType m_EndingWaterType;
	//是否片尾补视频帧(校正时长)
	bool m_IsAdjustDuration;
	unsigned char* m_LastFrameData;
    
    /**
     *  设置 I 帧的间隔
     */
    float mIFrameInternal = 1;
    
public:
	CFrameRecorder();
	~CFrameRecorder();

	int Open(const char* dstFile,int width,int height);
	//call this api before SetupCropRegion
	int SetupQuality(MediaRecorderQuality quality);
	//设置最大帧率为30帧
	//Start() 之前设置
	void SetMaxFrameRate30(bool isMax30fps);
	//设置裁剪区域
	int SetupCropRegion(int nLeft, int nTop, int nWidth, int nHeight,int rotate);
	//设置正常的水印Start() 之前设置
	//[watermark_path] 水印路径
	//[type] 水印的类型
	void SetWatermark(const char* watermark_path,MeipaiWatermarkType type = WATERMARK_TOP_LEFT);
	//设置片尾水印Start() 之前设置
	//[encode_duration] :编码的总时长
	//[type] 水印类型
	//[watermark_path] 水印路径
	void SetEndingWatermark(const char* watermark_path,MeipaiWatermarkType type ,double encode_duration);
	//setup record audio channle value: 1 or 2;
	//if no set the value is 1
	//call this api before SetupAudio 
	int SetupRecordChannle(int channle);
	//是否自动根据输入时长在片尾补视频帧Start() 之前设置
	//[isEnable] 表示是否开启这个功能
	//[encode_duration] 表示编码后的时长
	void AdjustVideoDuration(bool isEnable,double encode_duration);

    /**
     *  设置输入的Audio的格式参数
     *
     *  @param channle_count 声音通道数
     *  @param sample_rate   声音的采样率
     *  @param sample_fmt    声音的数据格式
     *
     *  @return 0 成功，其他失败。
     */
	int SetupAudio(int channle_count, int sample_rate, AVSampleFormat sample_fmt = AV_SAMPLE_FMT_S16P);
	//是否补齐音频Start() 之前设置
	void SetupAudioAligen(bool aligen = true);
	//设置填充模式,pixel = 上下要填充的像素
	void SetFillMode(EncodeFillMode mode,int pixel);

	int Start();

	int Record420SP(unsigned char* pYuv420sp, int width, int height, double time_stemp);

	int RecordI420(unsigned char* pY, int stride_y, unsigned char* pU, int stride_u,
		unsigned char* pV, int stride_v, int width, int height, double time_stemp);

	int RecordARGB(unsigned char* pARGB,int width,int height, double time_stemp);

	int RecordPCM(unsigned char** pData, int length);
	int RecordPCM(uint8* &pData, int length);

	int GetEncodeWidth();

	int GetEncodeHeight();

	int Finish();

	int Close();

	int startRecorderPrepare;
    
    /**
     *  Set I key frame internal.
     *  Default internal is 1s.
     *  Set {@link IFRAME_INTERNAL_AUTO} for encoder auto control.
     *  @param internal internal in seconds.
     */
    void setIFrameInternal(float internal);
};

