//
//  MTMacros.h
//  MVCoreV2
//
//  Created by Javan on 15/2/28.
//  Copyright (c) 2015年 Javan. All rights reserved.
//

#ifndef MVCoreV2_MTMacros_h
#define MVCoreV2_MTMacros_h


#define SAFE_DELETE(p)           do { delete (p); (p) = nullptr; } while(0)
#define SAFE_DELETE_ARRAY(p)     do { if(p) { delete[] (p); (p) = nullptr; } } while(0)
#define SAFE_FREE(p)             do { if(p) { free(p); (p) = nullptr; } } while(0)
#define SAFE_RELEASE(p)          do { if(p) { (p)->release(); } } while(0)
#define SAFE_RELEASE_NULL(p)     do { if(p) { (p)->release(); (p) = nullptr; } } while(0)
#define SAFE_RETAIN(p)           do { if(p) { (p)->retain(); } } while(0)
#define BREAK_IF(cond)           if(cond) break

// 计算数组的长度
#define NUMOFARRAYELEMENT(a)    (sizeof(a)/sizeof(a[0]))

// Assert macros.
#if !defined(NDK_DEBUG) || NDK_DEBUG == 0
#define GP_ASSERT(expression)
#else
#include <cassert>
#define GP_ASSERT(expression) assert(expression)
#endif

#ifndef FLT_EPSILON
#define FLT_EPSILON     1.192092896e-07F
#endif // FLT_EPSILON

#ifndef DBL_EPSILON
#define DBL_EPSILON     2.2204460492503131e-016
#endif // DBL_EPSILON

#if !defined(NDK_DEBUG) || NDK_DEBUG == 0
#define CHECK_GL_ERROR_DEBUG()

#if DEBUG == 1
#define MT_DEBUG
#endif

#else

#define MT_DEBUG

#define CHECK_GL_ERROR_DEBUG() \
do { \
    GLenum __error = glGetError(); \
        if(__error) { \
            Log::error("OpenGL error 0x%04X in %s %s %d\n", __error, __FILE__, __FUNCTION__, __LINE__); \
    } \
} while (false)
#endif

/// @name namespace glx
/// @{
#ifdef __cplusplus
#define NS_GLX_BEGIN                    namespace glx {
#define NS_GLX_END                       }
#define USING_NS_GLX                     using namespace glx
#define NS_GLX                           ::glx
#else
#define NS_GLX_BEGIN
#define NS_GLX_END
#define USING_NS_GLX
#define NS_GLX
#endif
//  end of namespace group
/// @}

#endif
