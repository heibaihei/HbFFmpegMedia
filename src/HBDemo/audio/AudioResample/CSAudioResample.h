//
//  CSAudioResample.h
//  FFmpeg
//
//  Created by zj-db0519 on 2017/8/15.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef CSAudioResample_h
#define CSAudioResample_h

#include <stdio.h>
#include <stdint.h>
#include <iostream>

#include "HBAudio.h"

namespace HBMedia {
    
typedef class CSAudioResample
{
public:
    CSAudioResample(AudioParams* targetAudioParam);
    ~CSAudioResample();
    
private:
    
protected:
    
private:
    AudioParams mTargetAudioParams;
    
} CSAudioResample;

}

#endif /* CSAudioResample_h */
