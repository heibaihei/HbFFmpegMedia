#pragma once

#include "gifdefine.h"
#include "define.h"

#define COLORTABLE_RED		0
#define COLORTABLE_GREEN	1
#define COLORTABLE_BLUE		2

typedef struct _NODE {
	BOOL bIsLeaf;               // TRUE if node has no children
	UINT nPixelCount;           // Number of pixels represented by this leaf
	UINT nRedSum;               // Sum of red components
	UINT nGreenSum;             // Sum of green components
	UINT nBlueSum;              // Sum of blue components
//		UINT nAlphaSum;             // Sum of alpha components
	struct _NODE* pChild[8];    // Pointers to child nodes
	struct _NODE* pNext;        // Pointer to next reducible node
} NODE;
	
class CQuantizer
{
public:
	CQuantizer (UINT nMaxColors, UINT nColorBits);
	virtual ~CQuantizer ();
	BOOL ProcessImage2 (BYTE* pImageData,int width,int height);
//	UINT GetColorCount ();
	void SetColorTable (BYTE* prgb);

protected:
	void AddColor (NODE** ppNode, BYTE r, BYTE g, BYTE b, UINT nColorBits,
		UINT nLevel, UINT* pLeafCount, NODE** pReducibleNodes);
	void* CreateNode (UINT nLevel, UINT nColorBits, UINT* pLeafCount,
		NODE** pReducibleNodes);
	void ReduceTree (UINT nColorBits, UINT* pLeafCount,
		NODE** pReducibleNodes);
	void DeleteTree (NODE** ppNode);
	void GetPaletteColors (NODE* pTree, BYTE* prgb, UINT* pIndex);
//	BYTE GetPixelIndex(long x,long y, int nbit, long effwdt, BYTE *pimage);
protected:
	NODE* m_pTree;
	UINT m_nLeafCount;
	NODE* m_pReducibleNodes[9];
	UINT m_nMaxColors;
	UINT m_nOutputMaxColors;
	UINT m_nColorBits;
};

