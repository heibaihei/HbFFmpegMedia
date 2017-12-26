#include "CSAudioDemo.h"
#include "CSAudioDecoder.h"
#include "CSAudioEncoder.h"


int CSAudioDemo_AudioDecoder() {
    /** 视频解码，将裸数据以内存缓存队列的方式输出解码数据，内部解码线程以最大的缓冲节点数缓冲解码后的数据，外部通过获取节点的方式进行取帧操作 */
    
    AudioParams targetAudioParam;
    audioParamInit(&targetAudioParam);
    targetAudioParam.sample_rate = 41000;
    
    HBMedia::CSAudioDecoder* pAudioDecoder = new HBMedia::CSAudioDecoder();
    pAudioDecoder->setInMediaType(MD_TYPE_COMPRESS);
    pAudioDecoder->setOutMediaType(MD_TYPE_RAW_BY_MEMORY);
    pAudioDecoder->setOutAudioMediaParams(targetAudioParam);
    pAudioDecoder->setInMediaFile((char *)CS_COMMON_RESOURCE_ROOT_PATH"/video/100.mp4");

    pAudioDecoder->prepare();
    pAudioDecoder->start();
    
    int HbErr = 0;
    AVFrame *pNewFrame = nullptr;
    while (true) {
        pNewFrame = nullptr;
        HbErr = pAudioDecoder->receiveFrame(&pNewFrame);
        switch (HbErr) {
            case 0:
                {
                    if (pNewFrame) {
                        if (pNewFrame->opaque)
                            av_freep(pNewFrame->opaque);
                        av_frame_free(&pNewFrame);
                    }
                }
                break;
            case -2:
            case -3:
                goto RECEIVED_END_LABEL;
            default:
                break;
        }
    }

RECEIVED_END_LABEL:
    pAudioDecoder->stop();
    pAudioDecoder->release();
    return HB_OK;
}

int CSAudioDemo_AudioEncoder()
{   /** 视频编码，将视频裸数据转换成编码数据文件输出 */
    HBMedia::CSAudioEncoder* pAudioEncoder = new HBMedia::CSAudioEncoder();
    pAudioEncoder->setInMediaType(MD_TYPE_RAW_BY_MEMORY);
    {
        AudioParams inAudioParamObj;
        audioParamInit(&inAudioParamObj);
        inAudioParamObj.channels = 2;
        inAudioParamObj.channel_layout = av_get_default_channel_layout(inAudioParamObj.channels);
        inAudioParamObj.sample_rate = 44100;
        inAudioParamObj.pri_sample_fmt = CS_SAMPLE_FMT_S16;
        inAudioParamObj.mbitRate = 64000;
        pAudioEncoder->setInAudioParams(inAudioParamObj);
    }
    {
        AudioParams outAudioParamObj;
        audioParamInit(&outAudioParamObj);
        outAudioParamObj.channels = 2;
        outAudioParamObj.channel_layout = av_get_default_channel_layout(outAudioParamObj.channels);
        outAudioParamObj.sample_rate = 44100;
        outAudioParamObj.mbitRate = 64000;
        outAudioParamObj.nb_samples = 1024;
        outAudioParamObj.pri_sample_fmt = CS_SAMPLE_FMT_FLTP;
        pAudioEncoder->setOutAudioParams(outAudioParamObj);
    }
    pAudioEncoder->setOutMediaType(MD_TYPE_COMPRESS);
    pAudioEncoder->setOutMediaFile((char *)AUDIO_RESOURCE_ROOT_PATH"/Encode/new100.mp3");
    
    pAudioEncoder->prepare();
    pAudioEncoder->start();
    
    {
        AudioParams targetAudioParam;
        audioParamInit(&targetAudioParam);
        targetAudioParam.sample_rate = 41000;
        targetAudioParam.pri_sample_fmt = CS_SAMPLE_FMT_S16;
        
        HBMedia::CSAudioDecoder* pAudioDecoder = new HBMedia::CSAudioDecoder();
        pAudioDecoder->setInMediaType(MD_TYPE_COMPRESS);
        pAudioDecoder->setOutMediaType(MD_TYPE_RAW_BY_MEMORY);
        pAudioDecoder->setOutAudioMediaParams(targetAudioParam);
        pAudioDecoder->setInMediaFile((char *)CS_COMMON_RESOURCE_ROOT_PATH"/video/100.mp4");

        pAudioDecoder->prepare();
        pAudioDecoder->start();
        int HbErr = HB_OK;
        AVFrame *pNewFrame = nullptr;
        while (true) {
            pNewFrame = nullptr;
            HbErr = pAudioDecoder->receiveFrame(&pNewFrame);
            switch (HbErr) {
                case 0:
                {
                    pAudioEncoder->sendFrame(&pNewFrame);
                }
                    break;
                case -2:
                case -3:
                    goto RECEIVED_END_LABEL;
                default:
                    break;
            }
        }
    RECEIVED_END_LABEL:
        pAudioDecoder->stop();
        pAudioDecoder->release();
    }
    
    pAudioEncoder->syncWait();
    pAudioEncoder->stop();
    pAudioEncoder->release();
    
    return HB_OK;
}
