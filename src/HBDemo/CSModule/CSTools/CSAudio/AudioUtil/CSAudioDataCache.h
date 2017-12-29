//
//  CSAudioDataCache.h
//  Sample
//
//  Created by zj-db0519 on 2017/12/29.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef CSAudioDataCache_h
#define CSAudioDataCache_h

#include <stdio.h>
#include "CSMediaBase.h"
namespace HBMedia {
 
typedef class CSAudioDataCache
{
    static int S_MAX_BUFFER_SIZE;
public:
    CSAudioDataCache();
    ~CSAudioDataCache();
    
    void setAudioParams(AudioParams *pAudioParam) { mAudioParams = *pAudioParam; }
    
    /**
     *  数据缓冲区的初始化
     *  @return HB_OK 初始化成功; HB_ERROR 初始化失败;
     */
    int CacheInitial();

    /**
     *  将得到的帧输入音频数据缓冲区，
     *  需要完整帧数据时，从缓冲区中读取数据;
     *  @param [in] pInFrame 待缓冲的数据帧
     *  @return 1   帧插入成功，并且得到相应的帧数据;
     *          0   帧数据缓冲失败;
     *          -1  无效调用
     */
    int WriteDataToCache(AVFrame *pInFrame);
    
    /**
     *  从音频缓冲区中读取一个完整的帧数据;
     *  @param [out] pOutFrame 读到的数据帧
     *  @return 1 读到一个完整的帧;
     *          0 目前无完整的音频帧数据;
     *         -1 异常调用;
     *         -2 音频数据缓冲区已经读取完毕;
     */
    int ReadDataFromCache(AVFrame **pOutFrame);
    
    /**
     *  刷新音频缓冲区, 音频缓冲区中读完一个数据帧;
     *  @param [out] pOutFrame 读到的数据帧
     *  @return 1 读到一个完整的帧;
     *          0 目前无完整的音频帧数据;
     *         -1 异常调用;
     *         -2 音频数据缓冲区已经读取完毕;
     */
    int FlushDataCache(AVFrame **pOutFrame);

    /** 释放缓冲区资源 */
    void release();

    /**
     *  返回当前缓冲了的音频数据大小, sample 为单位
     */
    int getCacheSize();
private:
    /**
     *  [内部接口] 从缓冲区中读取数据;
     *  @param [out] pOutFrame 读到的数据帧
     *  @parma [in] 要读取的数据大小
     *  @return 1 读到一个完整的帧;
     *          0 取帧失败;
     *         -1 异常调用;
     *         -2 音频数据缓冲区已经读取完毕;
     */
    int _ReadDataFromCache(AVFrame **pOutFrame, int samples);

private:
    AVAudioFifo *mCacheBuffer;
    AudioParams  mAudioParams;
    int64_t      mNextAudioFramePts;
    
} CSAudioDataCache;

}
#endif /* CSAudioDataCache_h */
