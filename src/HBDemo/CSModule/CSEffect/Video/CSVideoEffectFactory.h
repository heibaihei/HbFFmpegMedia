//
//  CSVideoEffectFactory.h
//  Sample
//
//  Created by zj-db0519 on 2017/11/15.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef CSVideoEffectFactory_h
#define CSVideoEffectFactory_h

#include <stdio.h>
#include "CSVideoBaseEffect.h"

namespace HBMedia {
    
class CSVideoEffectFactory {
public:
    CSVideoEffectFactory();
    ~CSVideoEffectFactory();
    static CSVideoBaseEffect *getVideoEffect(int type);
};
    
}

#endif /* CSVideoEffectFactory_h */
