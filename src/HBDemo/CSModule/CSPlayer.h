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

namespace HBMedia {
    
typedef class CSPlayer {
public:
    CSPlayer();
    ~CSPlayer();
    
    void setSaveMode(bool mode) { mbSaveMode=mode; }
    bool getSaveMode() { return mbSaveMode; }
protected:
    
private:
    bool mbSaveMode;
    
} CSPlayer;

}

#endif /* CSPlayer_h */
