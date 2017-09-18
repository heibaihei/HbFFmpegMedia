#include "MeiPaiGaussBlur.h"
#include "rotate_yuv.h"
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(x) { if (x) delete [] x; x = NULL; } 
#endif
#include <pthread.h>
#include <cmath>
#include <string.h>
#if defined(_ANDROID_) || defined(ANDROID)
#include <unistd.h>
#endif

#ifndef max
#define max(a,b)    (((a) > (b)) ? (a) : (b))
#define min(a,b)    (((a) < (b)) ? (a) : (b))
#endif

//////////////////////////////////////////////////////////////////////////
typedef struct DericheExParetemer
{
	int *a0Table;
	int *a1Table;
	int *arTable;
	int *cnTable;
	int *cpTable;
	byte *pImageStream;
	int nLineCount;
	int nLinePixelCount;
	int nLineStride;
	int nIntB1,nIntB2,nShift,halfShift;
	int *pCacheData;

}DericheExParetemer;

static void *DericheExPart(void* par)
{
	DericheExParetemer nPar = *(DericheExParetemer*)par;
	int *a0Table = nPar.a0Table;
	int *a1Table = nPar.a1Table;
	int *arTable = nPar.arTable;
	int *cnTable = nPar.cnTable;
	int *cpTable = nPar.cpTable;
	byte *pImageStream = nPar.pImageStream;
	int nLineCount = nPar.nLineCount;
	int nLinePixelCount = nPar.nLinePixelCount;
	int nIntB1 = nPar.nIntB1;
	int nIntB2 = nPar.nIntB2;
	int nShift = nPar.nShift;
	int halfShift = nPar.halfShift;
	int nLineStride = nPar.nLineStride;
	int* pCacheData = nPar.pCacheData;
	byte xp = 0, xc = 0;
	int yp = 0, yc =  0;
	int row,col;
	for(row=0; row<nLineCount; ++row)
	{
		byte* pCurImageAddr = pImageStream + row * nLineStride;
		int* pCurCacheAddr = pCacheData;
		xp = pCurImageAddr[0];
		yc = yp = cpTable[xp];
		for(col=0; col<nLinePixelCount; ++col)
		{
			xc = pCurImageAddr[0];
			pCurCacheAddr[0] = (a0Table[xc] + a1Table[xp] - ((nIntB1 * yc)>>halfShift) - ((nIntB2 * yp)>>halfShift));
			xp = xc;
			yp = yc;
			yc = pCurCacheAddr[0];

			pCurImageAddr++;
			pCurCacheAddr++;
		}

		pCurImageAddr --;
		pCurCacheAddr --;

		int ret ;
		xc = pCurImageAddr[0];
		xp = xc;
		yc = yp = cnTable[xc];

		for(col=0; col<nLinePixelCount; ++col)
		{
			ret = (arTable[xp|(xc<<8)] - ((nIntB1 * yc)>>halfShift) - ((nIntB2 * yp)>>halfShift));
			xp = xc;
			xc = pCurImageAddr[0];
			yp = yc;
			yc = ret;

			pCurImageAddr[0] = (pCurCacheAddr[0] + ret)>>nShift;
			pCurCacheAddr --;
			pCurImageAddr --;
		}
	}
	return NULL;
}
//获取CPU的个数
static inline int GetCPUCount()
{
#if defined(_ANDROID_) || defined(ANDROID)
	return min(4,sysconf (_SC_NPROCESSORS_ONLN ));
#else
	return 4;
#endif
	
}


//////////////////////////////////////////////////////////////////////////
void DericheEx(byte* pImageStream, int nWidth, int nHeight, float fRadius)
{
	// 公式参数计算
	float alpha = 1.695f / fRadius;
	float ema = exp(-alpha), ema2 = exp(-2*alpha);
	
	float b1 = -2*ema, b2 = ema2;
	
	float k = (1 - ema) * (1 - ema) / (1 + 2*alpha*ema - ema2);
	float a0 = k, a1 = k*(alpha - 1)*ema, a2 = k*(alpha + 1)*ema, a3 = -k*ema2;
	
	float coefp = (a0 + a1) / (1 + b1 + b2);
	float coefn = (a2 + a3) / (1 + b1 + b2);
	
	
	int nLineCount;		       // 行数
	int nLinePixelCount;    // 一行像素个数
	int nLineStride;		       // 行跨度
	
	nLineCount = nHeight;
	nLinePixelCount = nWidth;
	nLineStride = nWidth;

	
	// 临时数据存储行
	//int* pCacheData = (int*)(fbObj->fbHeap->VaryBuffer + nWidth*nHeight);// malloc(sizeof(int)*nLinePixelCount);//
	
	byte xp = 0, xc = 0;
	int yp = 0, yc =  0;
	int row,col;
	int  a0Table[256] ,a1Table[256],a3Table[256],a2Table[256],cnTable[256],cpTable[256];
	int arTable[65536];
 	//安全精度;
	const int nShift = 10;
	const int halfShift = 12;
	int mulFactor = 1<<nShift;
	int mulHalfFactor = 1<<halfShift;
	int nIntB1 = b1*mulHalfFactor;
	int nIntB2 = b2*mulHalfFactor;

	for(row = 0;row <256;row++)
	{
		a0Table[row] = a0*row*mulFactor;
		a1Table[row] = a1*row*mulFactor;
		a2Table[row] = a2*row*mulFactor;
		a3Table[row] = a3*row*mulFactor;
		cnTable[row] = coefn*row*mulFactor;
		cpTable[row] = coefp*row*mulFactor;
	}
	
	for(row=0;row<256;row++)
		for(col=0;col<256;col++)
			arTable[col|(row<<8)] = a2Table[row]+a3Table[col];

	int nCpu = GetCPUCount();

	//nCpu = min(4,nCpu);

	int* pCacheData = new int[nLinePixelCount*nCpu];
	//nCpu = min(nCpu,4);
	int i = 0;
	int shiftHeight = 0;
	int ThreadCount = 0;
	pthread_t* pThreadArray = NULL;
	DericheExParetemer* parArray = NULL;
	if(nCpu > 1)
	{
		shiftHeight = nLineCount/nCpu;
		ThreadCount = nCpu - 1;
		pThreadArray = new pthread_t[ThreadCount];
		parArray = new DericheExParetemer[ThreadCount];
		for(i = 0; i < ThreadCount; ++i)
		{
			int len = shiftHeight*nLineStride*i;
			parArray[i].pImageStream = pImageStream + len;
			parArray[i].nLinePixelCount = nLinePixelCount;
			parArray[i].pCacheData = pCacheData + i*nLinePixelCount;
			parArray[i].a0Table = a0Table;
			parArray[i].a1Table = a1Table;
			parArray[i].arTable = arTable;
			parArray[i].cnTable = cnTable;
			parArray[i].cpTable = cpTable;
			parArray[i].halfShift = halfShift;
			parArray[i].nIntB1 = nIntB1;
			parArray[i].nIntB2 = nIntB2;
			parArray[i].nLineCount = shiftHeight;
			parArray[i].nLineStride = nLineStride;
			parArray[i].nShift = nShift;
			if ( pthread_create( &pThreadArray[i], NULL, DericheExPart, &parArray[i]) ) {
				//LOGD("error pthread_create.");
			}
		}
	}
	DericheExParetemer nPar;
	nPar.pImageStream = pImageStream + shiftHeight*nLineStride*i;
	nPar.nLinePixelCount = nLinePixelCount;
	nPar.pCacheData = pCacheData + i*nLinePixelCount;
	nPar.a0Table = a0Table;
	nPar.a1Table = a1Table;
	nPar.arTable = arTable;
	nPar.cnTable = cnTable;
	nPar.cpTable = cpTable;
	nPar.halfShift = halfShift;
	nPar.nIntB1 = nIntB1;
	nPar.nIntB2 = nIntB2;
	nPar.nLineCount = nLineCount - shiftHeight*i;
	nPar.nLineStride = nLineStride;
	nPar.nShift = nShift;
	DericheExPart(&nPar);


	if(pThreadArray != NULL)
	{
		for(i = 0; i < ThreadCount; ++i)
		{
			if ( pthread_join (pThreadArray[i], NULL) ) {
				//LOGD("error joining thread.");
			}
		}

		SAFE_DELETE_ARRAY(parArray);
		SAFE_DELETE_ARRAY(pThreadArray);
	}
	/*for(row=0; row<nLineCount; ++row)
	{
		byte* pCurImageAddr = pImageStream + row * nLineStride;
		int* pCurCacheAddr = pCacheData;
		xp = pCurImageAddr[0];
		yc = yp = cpTable[xp];
		for(col=0; col<nLinePixelCount; ++col)
		{
			xc = pCurImageAddr[0];
			
			pCurCacheAddr[0] = (a0Table[xc] + a1Table[xp] - ((nIntB1 * yc)>>halfShift) - ((nIntB2 * yp)>>halfShift));
			xp = xc;
			yp = yc;
			yc = pCurCacheAddr[0];
			
			pCurImageAddr++;
			pCurCacheAddr++;
		}
		
		pCurImageAddr --;
		pCurCacheAddr --;
		
		int ret ;
		xc = pCurImageAddr[0];
		xp = xc;
		yc = yp = cnTable[xc];
		
		for(col=0; col<nLinePixelCount; ++col)
		{
			ret = (arTable[xp|(xc<<8)] - ((nIntB1 * yc)>>halfShift) - ((nIntB2 * yp)>>halfShift));
			xp = xc;
			xc = pCurImageAddr[0];
			yp = yc;
			yc = ret;
			
			pCurImageAddr[0] = ((pCurCacheAddr[0])>>nShift) + ((ret)>>nShift);
			pCurCacheAddr --;
			pCurImageAddr --;
		}
	}*/
	SAFE_DELETE_ARRAY(pCacheData);
	
}


int CreateBlurEffectInt( BYTE* pImageStream, int nWidth, int nHeight, float fXRadius, float fYRadius,int alpha)
{
	if (NULL == pImageStream)
	{
		return -1;
	}


	// 通道数
	//	int nChannelCount = 1;//nRowStride / nWidth;

	//	LOGD("zcd CreateBlurEffectInt 1");
	// 水平
	// 	if (nWidth > 1 && fXRadius >= MIN_CANNY_DERICHE_BLUR_RADIUS)
	// 	{
	//	fXRadius = min(fXRadius, MAX_CANNY_DERICHE_BLUR_RADIUS);
	byte* pBackupData = NULL;
	if(alpha != 100)
	{
		pBackupData = new byte[nWidth*nHeight];
		memcpy(pBackupData,pImageStream,nWidth*nHeight*sizeof(byte));
	}
	
	
	DericheEx(pImageStream, nWidth, nHeight, fXRadius);
	//	}
	//	LOGD("zcd CreateBlurEffectInt 2");
	byte* tmpStream = new byte[nWidth*nHeight];
	RotatePlane90(pImageStream,nWidth,tmpStream,nHeight,nWidth,nHeight);
	//	LOGD("zcd CreateBlurEffectInt 3");
	// 竖直
	// 	if (nHeight > 1 && fYRadius >= MIN_CANNY_DERICHE_BLUR_RADIUS)
	// 	{
	//	fYRadius = min(fYRadius, MAX_CANNY_DERICHE_BLUR_RADIUS);
	DericheEx(tmpStream, nHeight ,nWidth, fYRadius);
	//		LOGD("zcd CreateBlurEffectInt 4");
	//	}
	RotatePlane270(tmpStream,nHeight,pImageStream,nWidth,nHeight,nWidth);

	if(alpha != 100 && pBackupData)
	{
		int nPixelCount = nWidth*nHeight;
		for(int i = 0 ; i < nPixelCount ; i ++)
		{
			pImageStream[i] = ((100 - alpha)*pBackupData[i] + alpha* pImageStream[i])/100;
		}
	}

	SAFE_DELETE_ARRAY(tmpStream);
	SAFE_DELETE_ARRAY(pBackupData);
	//	LOGD("zcd CreateBlurEffectInt 5");
	return 1;
}

