#ifndef _H_FORMATCVT_ROW_DEFINE_H_
#define _H_FORMATCVT_ROW_DEFINE_H_
#include <stddef.h>
#include <stdlib.h>
#if defined(__ANDROID__) || (defined(_MSC_VER) && (_MSC_VER < 1600))
#include <sys/types.h>  // for uintptr_t on x86
#else
#include <stdint.h>  // for uintptr_t
#endif

#ifndef MT_RED
#define	MT_BLUE   2
#define	MT_GREEN  1
#define MT_RED    0
#define MT_ALPHA  3
//#define	MT_BLUE   3
//#define	MT_GREEN  2
//#define MT_RED    1
//#define MT_ALPHA  0
#endif

#define HAS_J422TOARGBROW_NEON
#define HAS_ARGBTOYJROW_NEON
#define HAS_ARGBTOUVJ422ROW_NEON
#define HAS_ARGBTOUVJROW_NEON

#define HAS_MERGEUVROW_NEON

#define HAS_NV21TOARGBROW_NEON
#define HAS_NV12TOARGBROW_NEON
#define HAS_ARGBTOYROW_NEON
#define HAS_ARGBTOUVROW_NEON
#define HAS_ARGBTOUV422ROW_NEON

#define HAS_I422TOYUY2JROW_NEON
#define HAS_YUY2JTOARGBROW_NEON

#define IS_ALIGNED(p, a) (!((uintptr_t)(p) & ((a) - 1)))
#define AVGB(a, b) (((a) + (b) + 1) >> 1)

#define MEMACCESS(a) 


#ifdef __cplusplus
#define align_buffer_64(var, size)                                             \
  uint8* var##_mem = reinterpret_cast<uint8*>(malloc((size) + 63));            \
  uint8* var = reinterpret_cast<uint8*>                                        \
      ((reinterpret_cast<intptr_t>(var##_mem) + 63) & ~63)
#else
#define align_buffer_64(var, size)                                             \
  uint8* var##_mem = (uint8*)(malloc((size) + 63));               /* NOLINT */ \
  uint8* var = (uint8*)(((intptr_t)(var##_mem) + 63) & ~63)       /* NOLINT */
#endif

#define free_aligned_buffer_64(var) \
  free(var##_mem);  \
  var = 0


typedef unsigned char uint8;
typedef unsigned int uint32;
typedef short int16;
typedef char int8;
typedef unsigned short uint16;
typedef int int32;
typedef long long int64;



#if defined(_MSC_VER) && !defined(__CLR_VER)
#define SIMD_ALIGNED(var) __declspec(align(16)) var
#define SIMD_ALIGNED32(var) __declspec(align(64)) var
typedef __declspec(align(16)) int16 vec16[8];
typedef __declspec(align(16)) int32 vec32[4];
typedef __declspec(align(16)) int8 vec8[16];
typedef __declspec(align(16)) uint16 uvec16[8];
typedef __declspec(align(16)) uint32 uvec32[4];
typedef __declspec(align(16)) uint8 uvec8[16];
typedef __declspec(align(32)) int16 lvec16[16];
typedef __declspec(align(32)) int32 lvec32[8];
typedef __declspec(align(32)) int8 lvec8[32];
typedef __declspec(align(32)) uint16 ulvec16[16];
typedef __declspec(align(32)) uint32 ulvec32[8];
typedef __declspec(align(32)) uint8 ulvec8[32];
#elif defined(__GNUC__)
// Caveat GCC 4.2 to 4.7 have a known issue using vectors with const.
#define SIMD_ALIGNED(var) var __attribute__((aligned(16)))
#define SIMD_ALIGNED32(var) var __attribute__((aligned(64)))
typedef int16 __attribute__((vector_size(16))) vec16;
typedef int32 __attribute__((vector_size(16))) vec32;
typedef int8 __attribute__((vector_size(16))) vec8;
typedef uint16 __attribute__((vector_size(16))) uvec16;
typedef uint32 __attribute__((vector_size(16))) uvec32;
typedef uint8 __attribute__((vector_size(16))) uvec8;
typedef int16 __attribute__((vector_size(32))) lvec16;
typedef int32 __attribute__((vector_size(32))) lvec32;
typedef int8 __attribute__((vector_size(32))) lvec8;
typedef uint16 __attribute__((vector_size(32))) ulvec16;
typedef uint32 __attribute__((vector_size(32))) ulvec32;
typedef uint8 __attribute__((vector_size(32))) ulvec8;
#else
#define SIMD_ALIGNED(var) var
#define SIMD_ALIGNED32(var) var
typedef int16 vec16[8];
typedef int32 vec32[4];
typedef int8 vec8[16];
typedef uint16 uvec16[8];
typedef uint32 uvec32[4];
typedef uint8 uvec8[16];
typedef int16 lvec16[16];
typedef int32 lvec32[8];
typedef int8 lvec8[32];
typedef uint16 ulvec16[16];
typedef uint32 ulvec32[8];
typedef uint8 ulvec8[32];

#endif



#endif