#include "CSAudioDemo.h"
#include "CSAudioDecoder.h"


int CSAudioDemo_AudioDecoder() {
    /** 视频解码，将裸数据以内存缓存队列的方式输出解码数据，内部解码线程以最大的缓冲节点数缓冲解码后的数据，外部通过获取节点的方式进行取帧操作 */
    
    AudioParams targetAudioParam;
    audioParamInit(&targetAudioParam);
    targetAudioParam.sample_rate = 48000;
    
    HBMedia::CSAudioDecoder* pAudioDecoder = new HBMedia::CSAudioDecoder();
    pAudioDecoder->setInMediaType(MD_TYPE_COMPRESS);
    pAudioDecoder->setOutMediaType(MD_TYPE_RAW_BY_MEMORY);
    pAudioDecoder->setOutAudioMediaParams(targetAudioParam);
    pAudioDecoder->setInMediaFile((char *)CS_COMMON_RESOURCE_ROOT_PATH"/audio/music1.mp3");

    pAudioDecoder->prepare();
    pAudioDecoder->start();
    AVFrame *pNewFrame = nullptr;
    
    while (true) {
        usleep(20);
    }
    
    if (pAudioDecoder->getStatus() & DECODE_STATE_PREPARED) {
        while (!(pAudioDecoder->getStatus() & DECODE_STATE_DECODE_END)) {
            pNewFrame = nullptr;
            if ((pAudioDecoder->receiveFrame(&pNewFrame) == HB_OK) && pNewFrame) {
                if (pNewFrame->opaque)
                    av_freep(pNewFrame->opaque);
                av_frame_free(&pNewFrame);
            }

            usleep(10);
        }
    }
    pAudioDecoder->stop();
    pAudioDecoder->release();
    return HB_OK;
}
