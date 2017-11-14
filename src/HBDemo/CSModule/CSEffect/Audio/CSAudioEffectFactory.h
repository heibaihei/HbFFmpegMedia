//
//  CSAudioEffectFactory.h
//  Sample
//
//  Created by zj-db0519 on 2017/11/14.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef CSAudioEffectFactory_h
#define CSAudioEffectFactory_h

#include <stdio.h>
#include "CSDefine.h"
#include "CSAudioBaseEffect.h"

namespace HBMedia {
    
class CSAudioEffectFactory {
public:
    CSAudioEffectFactory();
    
    ~CSAudioEffectFactory();
    
    static CSAudioBaseEffect *getAudioEffect(int type);
};
    
}


#endif /* CSAudioEffectFactory_h */
