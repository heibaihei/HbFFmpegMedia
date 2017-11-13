//
//  CSPlayerListener.h
//  Sample
//
//  Created by zj-db0519 on 2017/11/14.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef CSPlayerListener_h
#define CSPlayerListener_h

namespace MTMediaRecord {
    class MediaRecorder;
    
    
    class MediaRecorderStateListener {
    public:
        virtual ~MediaRecorderStateListener() {}
        /**
         *  监听录制开始
         *
         *  @param recorder MediaRecorder object.
         */
        virtual void MediaRecordProgressBegan(MediaRecorder* recorder) = 0;
        
        /**
         *  录制过程状态改变
         *
         *  @param recorder   MediaRecorder object.
         *  @param type 状态变迁的类型
         */
        virtual void MediaRecordProgressChanged(MediaRecorder* recorder, int type) = 0;
        
        /**
         *  录制结束
         *
         *  @param editer  MediaRecorder object.
         */
        virtual void MediaRecordProgressEnded(MediaRecorder* editer) = 0;
        
        /**
         *  取消录制消息
         *
         *  @param editer MediaRecorder object
         */
        virtual void MediaRecordProgressCanceled(MediaRecorder* editer) = 0;
    };
}


#endif /* CSPlayerListener_h */
