#include "CSAudioDemo.h"
#include "CSAudioDecoder.h"


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
