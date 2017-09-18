#ifndef _H_MEIPAI_GAUSSBLUR_H_
#define _H_MEIPAI_GAUSSBLUR_H_

typedef unsigned char byte;
typedef unsigned char BYTE;

void DericheEx(byte* pImageStream, int nWidth, int nHeight, float fRadius);

int CreateBlurEffectInt(BYTE* pImageStream, int nWidth, int nHeight, float fXRadius, float fYRadius, int alpha = 100);

#endif