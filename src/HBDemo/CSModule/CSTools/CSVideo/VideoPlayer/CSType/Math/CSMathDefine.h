#ifndef CSMATH_DEFINE_H
#define CSMATH_DEFINE_H

#include <memory>
#include <string.h>

#define MATH_DEG_TO_RAD(x)          ((x) * 0.0174532925f)
#define MATH_RAD_TO_DEG(x)          ((x)* 57.29577951f)

#define MATH_FLOAT_SMALL            1.0e-37f
#define MATH_TOLERANCE              2e-37f
#define MATH_PIOVER2                1.57079632679489661923f

#define MATH_EPSILON                0.000001f
#define NS_MT_MATH_BEGIN
#define NS_MT_MATH_END
#define USING_NS_MT_MATH

#define MATRIX_SIZE  (sizeof(float) * 16)

#endif // CSMATH_DEFINE_H
