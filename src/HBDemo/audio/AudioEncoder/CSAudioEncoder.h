//
//  CSAudioEncoder.h
//  FFmpeg
//
//  Created by zj-db0519 on 2017/8/15.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef CSAudioEncoder_h
#define CSAudioEncoder_h

#include <stdio.h>
#include <stdint.h>
#include <iostream>

#include "HBAudio.h"

namespace HBMedia {

typedef class CSAudioEncoder
{
public:
    CSAudioEncoder(AudioParams* targetAudioParam);
    ~CSAudioEncoder();
    
private:
    
protected:
    
private:
    AudioParams mTargetAudioParams;
    
} CSAudioEncoder;
    
} /** HBMedia */

#endif /* CSAudioEncoder_h */
