//
//  CSPlayer.h
//  FFmpeg
//
//  Created by zj-db0519 on 2017/9/10.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef CSPlayer_h
#define CSPlayer_h

#include <stdio.h>
#include "CSDefine.h"
#include "CSLog.h"
#include "CSDefine.h"
#include "CSCommon.h"
#include "CSUtil.h"


namespace HBMedia {

class MediaStateListener;
class CSTimeline;
    
typedef class CSPlayer {
public:
    CSPlayer();
    ~CSPlayer();
    
    void setSaveMode(bool mode) { mbSaveMode=mode; }
    bool getSaveMode() { return mbSaveMode; }
    
    void setTimeline(CSTimeline* pNewTimeline) { mTimeline = pNewTimeline; }
    CSTimeline* getTimeline() { return mTimeline; }
    /**
     *  完成媒体流的构建后，调用准备接口
     *  @return HB_OK 为正常, HB_ERROR 为异常;
     */
    int prepare();
    
    int start();
    
    int stop();
    
    int release(void);
    
    /** 临时接口，往播放器中喂数据 */
    int writeExternData(uint8_t data[], size_t dataSize, int index, long timeStamp);
    
    /** 设置监听器 */
    void setStateListener(MediaStateListener *listener);
    
protected:
    
    
private:
    /** 状态监听器 */
    MediaStateListener *mStateListener;
    
    /** 是否处于保存模式: true处于保存模式, false处于预览模式 */
    bool mbSaveMode;
    /** 
     *  装有素材的 timeline 结构
     */
    CSTimeline* mTimeline;
    unsigned long long mStatues;
    
} CSPlayer;

}

#endif /* CSPlayer_h */
