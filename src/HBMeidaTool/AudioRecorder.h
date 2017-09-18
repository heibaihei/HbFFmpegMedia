// Create by yzh 2015-01-06
// AudioRecorder.h
// 
/* Use Sample
CAudioEditer editor;
editor.Open("meipai_20150106143643.amr"); //srcpath
double dura = editor.GetAudioDuration();
editor.CutAudioWithTime("res.mp3", 0, dura); //dstpath
editor.Close();
*/
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

class CAudioRecorder
{
public:
	CAudioRecorder();
	~CAudioRecorder();
	int Open(const char* dstFile);
	int SetupAudio(int channle_count, int sample_rate, AVSampleFormat sample_fmt = AV_SAMPLE_FMT_S16P);
	int Start();
	int RecordPCM(unsigned char** pData, int length);
	int Finish();
	int Close();
protected:
	char m_FilePath[300];
	AVFormatContext *m_pOutputFormatContext;
	AVOutputFormat *m_pOutputFormat;
	AVStream* m_pDstAudioStream;
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
	int src_sample_rate;

};

