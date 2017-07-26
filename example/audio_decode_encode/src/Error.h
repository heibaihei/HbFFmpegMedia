//
//  Error.h
//  admanagerment
//
//  Created by 薛庆明 on 29/04/2017.
//  Copyright © 2017 meitu. All rights reserved.
//

#ifndef Error_h
#define Error_h
#include "Common.h"

#ifndef makeErrorStr
static char errorStr[AV_ERROR_MAX_STRING_SIZE];
#define makeErrorStr(errorCode) av_make_error_string(errorStr, AV_ERROR_MAX_STRING_SIZE, errorCode)
#endif


typedef enum ErrorCode {
    AV_STAT_ERR     = -100,
    AV_NOT_INIT     = -99,
    AV_FILE_ERR     = -98,
    AV_STREAM_ERR   = -97,
    AV_MALLOC_ERR   = -96,
    AV_DECODE_ERR   = -95,
    AV_SEEK_ERR     = -94,
    AV_PARM_ERR     = -93,
    AV_NOT_FOUND    = -92,
    AV_SET_ERR      = -91,
    AV_CONFIG_ERR   = -90,
    AV_ENCODE_ERR   = -89,
    AV_TS_ERR       = -88,
    AV_FIFO_ERR     = -87,
    AV_NOT_SUPPORT  = -86,
    AV_NOT_ENOUGH   = -85,
    AV_EXIT_NORMAL       = 1,
} ErrorCode;


#endif /* Error_h */
