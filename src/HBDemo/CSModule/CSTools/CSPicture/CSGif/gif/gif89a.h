#if !defined(_GIF89A_)
#define _GIF89A_

#include <iostream>
#include <fstream>
#include <string>
using namespace std;

#include "gifdefine.h"
#include "define.h"


typedef struct
{	BOOL active;
	UINT disposalMethod;
	BOOL userInputFlag;
	BOOL trsFlag;
	WORD delayTime;
	UINT trsColorIndex;
}GCTRLEXT;

typedef struct 
{	WORD imageLPos;
	WORD imageTPos;
	WORD imageWidth;
	WORD imageHeight;
	BOOL lFlag;
	BOOL interlaceFlag;
	BOOL sortFlag;
	UINT lSize;
	BYTE *pColorTable;
	BYTE *dataBuf;
	GCTRLEXT ctrlExt;
}FRAME;
typedef FRAME *LPFRAME;
//typedef const FRAME *LPCFRAME;
typedef FRAME *LPCFRAME;//zk

typedef struct 
{	UINT frames;
	WORD scrWidth,scrHeight;
	BOOL gFlag;
	UINT colorRes;
	BOOL gSort;
	UINT gSize;
	UINT BKColorIdx;
	UINT pixelAspectRatio;
	BYTE *gColorTable;
}GLOBAL_INFO;
typedef GLOBAL_INFO *LPGLOBAL_INFO;
typedef const GLOBAL_INFO *LPCGLOBAL_INFO;

typedef struct
{	
	UINT len;
	unsigned char* p;
}STRING_TABLE_ENTRY;



class CGif89a
{
private:
	UINT checkFrames();
	BOOL getAllFrames();
	BOOL extractData(FRAME* f);
	BOOL initStrTable(STRING_TABLE_ENTRY* strTable,UINT rootSize);
	BOOL addStrTable(STRING_TABLE_ENTRY* strTable,UINT addIdx,UINT idx,unsigned char c);

	LPCFRAME getNextFrame();
public:
	CGif89a();
	CGif89a(char* fileName,BOOL inMem);
	~CGif89a();
	BOOL operator!();
	BOOL open(char* fileName,BOOL inMem);
	void close();
	char* getVer();
	
	BYTE* getNextFrameT(int *deley);
	LPCGLOBAL_INFO getGlobalInfo();
	
	int getWidth();
	int getHeight();
	int getFrameNum();
	int getDelay(int id);
private:
	char* m_strOpenPath;
//	ifstream m_file;
	FILE* m_file;
	char* m_gifPath;
	char m_version[4];
	BOOL m_error;
	BOOL m_opened;
	BOOL m_inMem;
	BYTE m_bColorTable[256*3];
//	BYTE m_lColorTable[256*3];
	streampos m_dataStart;
	FRAME *m_allFrames;
	UINT m_curIndex;
	
	GLOBAL_INFO m_gifInfo;
	FRAME m_curFrame;
	GCTRLEXT m_ctrlExt;
};


extern CGif89a* g_gifReader;
#endif