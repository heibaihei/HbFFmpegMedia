#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include "InFileContext.hpp"
#include "Resampler.hpp"
#ifdef __cplusplus
}
#endif
class CAudioEditer
{
public:
	CAudioEditer();
	~CAudioEditer();
    
private:
    InFileContext *fileCtx;
    AudioDecoder *audioDecoder;
    Resampler *resample;
	//src decode frame
	AVFrame* m_pSrcFrame;
	//audio stream index
	int m_audio_stream_idx;

    int channels;
    int sampleRate;
    double duration;
    
    unsigned int resampleSize;
    
public:
	//open a audio.
	int Open(const char* file);
	//get audio duration
	double GetAudioDuration();
    // get audio channels
    int GetAudioChannels();
    
    // get audio sample rate
    int GetAudioSampleRate();
	// copy decoded data  to out buffer from file
    int CopyDecodeFrameDataFromFile(short *audioData, size_t dataSize);
	//close current audio.
	void Close();
    
private:
    

    
};

