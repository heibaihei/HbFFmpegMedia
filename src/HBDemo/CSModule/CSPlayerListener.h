//
//  CSPlayerListener.h
//  Sample
//
//  Created by zj-db0519 on 2017/11/14.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef CSPlayerListener_h
#define CSPlayerListener_h

namespace HBMedia {
    class CSPlayer;
    
    
    class MediaStateListener {
    public:
        virtual ~MediaStateListener() {}
        /**
         *  监听录制开始
         *
         *  @param recorder MediaRecorder object.
         */
        virtual void MediaRecordProgressBegan(CSPlayer* recorder) = 0;
        
        /**
         *  录制过程状态改变
         *
         *  @param recorder   MediaRecorder object.
         *  @param type 状态变迁的类型
         */
        virtual void MediaRecordProgressChanged(CSPlayer* recorder, int type) = 0;
        
        /**
         *  录制结束
         *
         *  @param editer  MediaRecorder object.
         */
        virtual void MediaRecordProgressEnded(CSPlayer* editer) = 0;
        
        /**
         *  取消录制消息
         *
         *  @param editer MediaRecorder object
         */
        virtual void MediaRecordProgressCanceled(CSPlayer* editer) = 0;
    };
}


#endif /* CSPlayerListener_h */
