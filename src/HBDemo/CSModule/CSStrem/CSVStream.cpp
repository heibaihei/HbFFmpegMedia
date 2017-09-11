//
//  CSVStream.cpp
//  FFmpeg
//
//  Created by zj-db0519 on 2017/9/11.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSVStream.h"

namespace HBMedia {
    
CSVStream::CSVStream(){
    
}

CSVStream::~CSVStream(){
    
}

void CSVStream::EchoStreamInfo() {
}

int CSVStream::sendRawData(uint8_t* pData, long DataSize, int64_t TimeStamp) {
    return HB_OK;
}

int CSVStream::bindOpaque(void *handle) {
    return HB_OK;
}

}
