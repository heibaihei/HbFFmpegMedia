#pragma once
/************************************************************************/
/*��480*480 ����Ƶ�ü���
/* Creator:	 yzh
/* Data:	 2015-09-17
/*Version:	 1.0.0
/************************************************************************/
#include "VideoEditer.h"
#include "FrameRecorder.h"
//������Ҫ�����߻���Ҫ����С��
#ifndef MT_IMPORT_MAX_SIZE
typedef enum MTVideoImportMode{
    MT_IMPORT_NON                   = -1,
	MT_IMPORT_MAX_SIZE              = 0,
	MT_IMPORT_MIN_SIZE              = 1,
    MT_IMPORT_MIN_SIZE_MULTIPLE_16  = 2,
    MT_IMPORT_FREE                  = 3,
}MTVideoImportMode;
#endif

typedef enum MTTargetVideoSize{
    MT_TARGET_VIDEO_1_1,
    MT_TARGET_VIDEO_3_4,
    MT_TARGET_VIDEO_4_3,
    MT_TARGET_VIDEO_16_9,
    MT_TARGET_VIDEO_9_16
}MTTargetVideoSize;


class CVideoEditerAny;

class VideoEditerAnyProgressListener {
    
public:
    virtual ~VideoEditerAnyProgressListener() {}
    /**
     *  Call When editer began process video
     *
     *  @param editer CVideoEditerAny object.
     */
    virtual void videoEditerAnyProgressBegan(CVideoEditerAny* editer) = 0;
    
    /**
     *  Call in progcess
     *
     *  @param editer   CVideoEditerAny object.
     *  @param progress
     *  @param total all duration.
     */
    virtual void videoEditerAnyProgressChanged(CVideoEditerAny* editer, double progress, double total) = 0;
    
    /**
     *  Call finish video process!
     *
     *  @param editer  CVideoEditerAny object.
     */
    virtual void videoEditerAnyProgressEnded(CVideoEditerAny* editer) = 0;
    
    /**
     *  Call canceled video process
     *
     *  @param editer CVideoEditerAny object
     */
    virtual void videoEditerAnyProgressCanceled(CVideoEditerAny* editer) = 0;
};

class CVideoEditerAny
{
public:
	CVideoEditerAny(void);
	~CVideoEditerAny(void);
public:
	//open a video.
	int Open(const char* file);
	//get video duration
	double GetVideoDuration();
	//get video duration
	double GetAudioDuration();
    //获取进度值[0.0-100.0]
    double GetProgressbarValue();
    //重置进度值为0
    void ClearProgressBarValue();
    //中断导入
    void Interrupt();
    //设置最终裁剪的边长
    //@param mode 表示是要按最小边是size还是最大边是size
    //set mode after open
    bool SetImportMode(MTVideoImportMode mode,int size);
    //设置裁剪区域
    bool SetCropRegion(int left, int top, int width, int height);
    //是否要加水印,不加传NULL
    void SetWaterMark(const char* watermark_path,MeipaiWatermarkType type = WATERMARK_TOP_LEFT);
    //加片尾水印.
    void SetEndingWaterMark(const char* watermark_path,MeipaiWatermarkType type = WATERMARK_CENTER);
    //裁剪;
    int CutVideoWithTime(const char* dstFile, double startTime, double endTime);
    //添加彩色边框
    int CutVideoWithFrame(const char* dstFile, double startTime, double endTime,
                          unsigned char r,unsigned char g,unsigned char b,MTTargetVideoSize sizeMode = MT_TARGET_VIDEO_1_1);
	//close current video.
	void Close();

    // 实现分段解码的API
    /**
     *  将开启的文件Seek到预期的位置
     *
     *  @param timeUs 按时间seek
     *
     *  @return 返回是否seek成功
     */
    int seekTo(int64_t timeUs);
    
    int getVideoTrakIndex();
    
    int getAudioTrackIndex();
    
    int readSample(uint8* buffer);
    
    int decodeVideo(uint8* buffer, int32_t size, int64_t &timeSample, int32_t &outSize);
    
    int32_t getOutputBufferSize();
    
    int getSampleTrackIndex();
    
    int64_t getSampleTime();
    
    int getSampleFlags();
    
    /**
     *  Advance to the next sample. Returns false if no more sample data 
     *  is available (end of stream).
     */
    bool advance();
    
    /**
     *  获取实际输出视频的宽度和高度
     *
     *  @return 返回宽度和高度的pair对象
     */
    const std::pair<int, int> getRealOutputSize();
    
    void setProgressListener(VideoEditerAnyProgressListener *listener);
    VideoEditerAnyProgressListener* getProgressListener();
private:
	//�򿪵���Ƶ�ļ����
	AVFormatContext* m_pFormatContext;
	//��Ƶ��
	AVStream *m_pVideoStream;
	//��Ƶ��
	AVStream *m_pAudioStream;
	//��Ƶcodec
	AVCodecContext *m_pVideoCodec;
	//��Ƶcodec
	AVCodecContext *m_pAudioCodec;
	//src decode frame
	AVFrame* m_pSrcFrame;
	//yuv420p frmae for scale
	AVFrame* m_YuvFrame;
	//420sp sws
	struct SwsContext* yuv_sws_ctx;
	//video stream index
	int m_video_stream_idx;
	//audio stream index
	int m_audio_stream_idx;
	//src video width
	int m_VideoWidth;
	//src video height
	int m_VideoHeight;
	//dst video width
	int m_DstVideoWidth;
	//dst video height
	int m_DstVideoHeight;
	//real encode width
	int m_RealEncodeWidth;
	int m_RealEncodeHeight;
	
    // clip region params
    int clipX, clipY;
    int clipWidth, clipHeight;

	//is exit audio
	bool m_ExitAudio;
	//rotate information
	MTVideoRotateType m_Rotate;
	//mode
	MTVideoImportMode m_ImportMode;
	//size
	int m_StandSize;
	//ˮӡ����
	MeipaiWatermarkType m_WatermarkType;
	//ˮӡ·��
	char* m_WatermarkPath;
	//ˮӡ����
	MeipaiWatermarkType m_EndingWatermarkType;
	//ˮӡ·��
	char* m_EndingWatermarkPath;
	//������ֵ
	double m_ProcessBar;
	//�Ƿ��ж�
	bool m_bInterrupt;
    
    // add by Javan buffer for read
    AVPacket packet;
    unsigned char* pResYuvData = NULL;
    RotationMode nYuvRotateMode = kRotate0;
    int32_t resYuvDataSize;
    
    VideoEditerAnyProgressListener *listener;
};

