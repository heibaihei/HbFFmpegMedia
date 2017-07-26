//
//  main.c
//  ffserver
//
//  Created by zj-db0519 on 2017/5/1.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include <stdio.h>

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

int main(int argc, const char * argv[]) {
    // insert code here...
    printf("Hello, World!\n");
    
    {
        char *filename = NULL;
        static AVFormatContext *gFmt_ctx = NULL;
        avformat_open_input(&gFmt_ctx, filename, NULL, NULL);
        avformat_find_stream_info(gFmt_ctx, NULL);
        
        
        
    }
    return 0;
}
