//
//  CSStreamFactory.cpp
//  FFmpeg
//
//  Created by zj-db0519 on 2017/9/11.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSStreamFactory.h"
#include "CSAStream.h"
#include "CSVStream.h"

namespace HBMedia {
 
CSIStream* CSStreamFactory::CreateMediaStream(STREAM_TYPE type) {
    CSIStream *pStream = NULL;
    
    switch (type) {
        case CS_STREAM_TYPE_VIDEO:
            pStream = new CSVStream();
            break;
            
        case CS_STREAM_TYPE_AUDIO:
            pStream = new CSAStream();
            break;
        default:
            break;
    }
    
    return pStream;
}

}
