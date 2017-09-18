#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#ifdef __cplusplus
}
#endif

#include <stack>
#include <vector>
using namespace std;

typedef struct EditorEntity
{
	double pts;
	unsigned char* pV;
	int stride_v;
	unsigned char* pU;
	int stride_u;
	unsigned char* pY;
	int stride_y;
	int width;
	int height;
}EditorEntity;
//create EditorEntity from  AVFrame
EditorEntity* EditorEntityCreate(AVFrame* frame,double time_stemp);
//release all entity data
void EditorEntityRelease(EditorEntity** pEntity);
typedef struct TimeNode{
	double start;
	double end;
	TimeNode(){}
	TimeNode(double s,double e)
	{
		start = s;
		end = e;
	}
}TimeNode;

typedef struct AudioEntity
{
	int nb_samples;
	uint8_t** data;
	int channels;
	double pts;
	AudioEntity()
	{
		data = NULL;
	}
}AudioEntity;

AudioEntity* AudioEntityCreate( AVFrame* frame ,double time_stemp);

void AudioEntityRelease(AudioEntity** pEntity);



typedef enum MTVideoRotateType{
	MT_VIDEO_ROTATE_0 = 0,
	MT_VIDEO_ROTATE_90 = 90,
	MT_VIDEO_ROTATE_180 = 180,
	MT_VIDEO_ROTATE_270 = 270
}MTVideoRotateType;

class CVideoEditer;

class VideoEditerProgressListener {
    
public:
    virtual ~VideoEditerProgressListener() {};
    /**
     *  Call When editer began process video
     *
     *  @param editer CVideEditer object.
     */
    virtual void videoEditerProgressBegan(CVideoEditer* editer) = 0;
    
    /**
     *  Call in progcess
     *
     *  @param editer   CVideEditer object.
     *  @param progress 
     *  @param total all duration.
     */
    virtual void videoEditerProgressChanged(CVideoEditer* editer, double progress, double total) = 0;
    
    /**
     *  Call finish video process!
     *
     *  @param editer  CVideEditer object.
     */
    virtual void videoEditerProgressEnded(CVideoEditer* editer) = 0;
};

class CVideoEditer
{
public:
	CVideoEditer();
	~CVideoEditer();
private:
	AVFormatContext* m_pFormatContext;
	AVStream *m_pVideoStream;
	AVStream *m_pAudioStream;
	AVCodecContext *m_pVideoCodec;
	AVCodecContext *m_pAudioCodec;
	//src decode frame
	AVFrame* m_pSrcFrame;
	//for save rgba frame
	AVFrame* m_pRGBFrame;
	//yuv420p frmae
	AVFrame* m_YuvFrame;
	//for save frame.
	struct SwsContext* rgb_sws_ctx;
	//420sp sws
	struct SwsContext* yuv_sws_ctx;

	//video stream index
	int m_video_stream_idx;
	//audio stream index
	int m_audio_stream_idx;

	int m_VideoWidth;
	int m_VideoHeight;
	int m_DstVideoWidth;
	int m_DstVideoHeight;
	int m_KeyFrameWidth;
	int m_KeyFrameHeight;
	int m_Left;
	int m_Top;

	double m_Duration;
	double m_KeyFrameStep;
	bool m_ExitAudio;
	unsigned char* m_pYuvData;
	//for rotate
	MTVideoRotateType m_Rotate;
    
    VideoEditerProgressListener *listener;

	stack<EditorEntity*> m_CutFrames;
    
    bool any_working = false;
    bool abort_request = false;
    
    static int sws_mode;
public:
    static void SetVideoEditerSwsMode(int mode);
    
    static void SetVideoEditerOutputSize(int width, int height);
    /**
     *  中断VideoEditer正在处理的任何动作
     */
    void abort();
    /**
     *  如果某种功能还在工作的时候返回false，如果没有任何工作则返回 true
     *
     *  @return true or false。
     */
    bool isAborted();
    
	//open a video.
	int Open(const char* file);
    
    void setProgressListener(VideoEditerProgressListener *listener);
    VideoEditerProgressListener* getProgressListener();
    
	//���û�ȡ�Ĺؼ�֡��ͼƬ��С
	int SetKeyFrameSize(int width, int height);
	//get video duration
	double GetVideoDuration();
    
    int getVideoRotation();
    
	//get video duration
	double GetAudioDuration();
	//get the video width
	int GetVideoWidth();
	//get the video height
	int GetVideoHeight();
    
    /**
     *  Get video stream bitrate.
     *
     *  @return video bitrate value.
     */
    int getVideoBitrate();
    
	//get show video width with rotate
	int GetShowWidth();
	//get show video height with rotate
	int GetShowHeight();
	//set crop left and top
	int SetLeftTop(int left, int top);
	//set the key frame step
	void SetKeyFrameStep(double step) { m_KeyFrameStep = step; }
	//get all key frame.
	//floder for save path.
	int GetAllKeyFrame(const char* floder);
	//cut video (second).
	//return save frame count
//	int CutVideoWithTime(const char* dstFile, double startTime, double endTime);
	//��Ӳ�ɫ�߿�
	int CutVideo(const char* dstFile, double startTime, double endTime, unsigned char r = 0,unsigned char g = 0,unsigned char b = 0);
//	int CutVideoWithFrame(const char* dstFile, double startTime, double endTime, unsigned char r = 0,unsigned char g = 0,unsigned char b = 0);
	//
	int ReCutVideoWithTime(const char* dstFile, double startTime, double endTime);
	//����,times = �����Ĵ���,isReverse = ÿһ���Ƿ��е���;
	//endTime-startTime��Ҫ����1��
	int OtoMad(const char* dstFile,double startTime,double endTime,int times,bool isReverse = false);
	//����һ��ʱ���;�����ĳ���ļ�;
	int OtoMad(const char* dstFile,vector<TimeNode>& timeNodes);
	//close current video.
	void Close();
private:
	unsigned char* CropI420(unsigned char* pY, int stride_y, unsigned char* pU, int stride_u,
		unsigned char* pV, int stride_v, int width, int height);
	//���ڰױ߿�
	void FillVideoFrame(unsigned char* pFillRes,AVFrame * frame,unsigned char cY,unsigned char cB,unsigned char cR);
    void FillVideoFrame(unsigned char* pFillRes,
                        uint8_t* pDstY, int dst_stride_y,
                        uint8_t* pDstU, int dst_stride_u,
                        uint8_t* pDstV, int dst_stride_v,
                        int dstWidth, int dstHeight,
                        unsigned char cY,unsigned char cB,unsigned char cR);
};

