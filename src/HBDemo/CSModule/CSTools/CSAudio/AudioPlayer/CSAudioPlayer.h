//
//  CSAudioPlayer.h
//  Sample
//
//  Created by zj-db0519 on 2018/1/3.
//  Copyright © 2018年 meitu. All rights reserved.
//

#ifndef CSAudioPlayer_h
#define CSAudioPlayer_h

#include <stdio.h>
#include "CSAudio.h"
#include "CSMediaBase.h"
#include "CSThreadContext.h"
#include "frame.h"
#include "CSFiFoQueue.h"

typedef struct _rbuf_s rbuf_t;

namespace HBMedia {

typedef class CSAudioPlayer : public CSMediaBase {
    
    static void CSAudioCallback(void *opaque, uint8_t *pOutDataBuffer, int outLength);
    
public:
    CSAudioPlayer();
    ~CSAudioPlayer();
    
    /**
     *  准备工作，对外接口
     */
    int prepare();
    
    int pause();
    
    int start();
    
    int sendFrame(AVFrame *pFrame);
    
protected:
    
    int _pause(bool bDoPause);
    
    /**
     * 打开音频设备
     * @return > 0 音频设备打开成功;
     *         <=0 音频设备打开失败
     */
    int _Open();
    
private:
    unsigned int    mTmpAudioDataBufferSize;
    uint8_t        *mTmpAudioDataBuffer;
    float           mVolume;
    int64_t         mAudioPlayedBytes;
    int             mAudioDataRoundBufferSize;
    rbuf_t*         mAudioDataRoundBuffer;
    pthread_mutex_t mAudioDataBufferMutex;
    pthread_cond_t  mAudioDataBufferCond;
} CSAudioPlayer;

}
#endif /* CSAudioPlayer_h */
