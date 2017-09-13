//
//  CSWorkTasks.hpp
//  FFmpeg
//
//  Created by zj-db0519 on 2017/9/12.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef CSWorkTasks_h
#define CSWorkTasks_h

#include <stdio.h>

namespace HBMedia {
    
typedef class CSWorkTasks {
public:
    static void *WorkTask_EncodeFrameRawData(void *arg);
    
    static void *WorkTask_WritePacketData(void *arg);
protected:
    
private:
    
} CSWorkTasks;
    
}


#endif /* CSWorkTasks_h */
