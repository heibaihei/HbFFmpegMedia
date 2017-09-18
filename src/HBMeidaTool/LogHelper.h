//
//  LogHelper.h
//  mtmv
//
//  Created by cwq on 15/7/29.
//  Copyright (c) 2015年 meitu. All rights reserved.
//

#ifndef mtmv_LogHelper_h
#define mtmv_LogHelper_h

//#include "CSLog.h"

#define DEBUG_NATVIE

#ifdef DEBUG_NATVIE

#define  LOGV(...)
#define  LOGD(...)
#define  LOGI(...)
#define  LOGW(...)
#define  LOGE(...)

#else // !DEBUG_NATVIE

#define  LOGV(...)
#define  LOGD(...)
#define  LOGI(...)
#define  LOGW(...)
#define  LOGE(...)

#endif // !DEBUG_NATVIE

#endif
