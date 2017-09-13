//
//  CSStreamFactory.hpp
//  FFmpeg
//
//  Created by zj-db0519 on 2017/9/11.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef CSStreamFactory_h
#define CSStreamFactory_h

#include <stdio.h>
#include "CSIStream.h"

namespace HBMedia {

typedef class CSStreamFactory {
public:
    static CSIStream* CreateMediaStream(STREAM_TYPE type);
} CSStreamFactory;

}

#endif /* CSStreamFactory_h */
