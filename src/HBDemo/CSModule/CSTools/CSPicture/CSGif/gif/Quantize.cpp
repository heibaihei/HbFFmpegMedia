//#include <windows.h>

#include "Quantize.h"
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
/////////////////////////////////////////////////////////////////////////////
CQuantizer::CQuantizer (UINT nMaxColors, UINT nColorBits)
{
	m_nColorBits = nColorBits < 8 ? nColorBits : 8;

	m_pTree	= NULL;
	m_nLeafCount = 0;
	for	(int i=0; i<=(int) m_nColorBits; i++)
		m_pReducibleNodes[i] = NULL;
	m_nMaxColors = m_nOutputMaxColors = nMaxColors;
	if (m_nMaxColors<16) m_nMaxColors=16;
}
/////////////////////////////////////////////////////////////////////////////
CQuantizer::~CQuantizer	()
{
	if (m_pTree	!= NULL)
		DeleteTree (&m_pTree);
}
/////////////////////////////////////////////////////////////////////////////
BOOL CQuantizer::ProcessImage2 (BYTE* pImageData,int width,int height)
{
	if (pImageData == NULL)
		return FALSE;
	
	int	i, j;
	
		for	(i=0; i<height;	i++) 
		{
			for	(j=0; j<width; j++)
			{
				AddColor (&m_pTree,	pImageData[MT_RED], pImageData[MT_GREEN], pImageData[MT_BLUE],
					m_nColorBits, 0, &m_nLeafCount,
					m_pReducibleNodes);
				while (m_nLeafCount	> m_nMaxColors)
					ReduceTree (m_nColorBits, &m_nLeafCount, m_pReducibleNodes);

				pImageData += 4;
			}
		}

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
void CQuantizer::AddColor (NODE** ppNode, BYTE r, BYTE g, BYTE b,
						   UINT nColorBits, UINT nLevel, UINT*	pLeafCount,	NODE** pReducibleNodes)
{
	BYTE	mask[8]	= {	0x80, 0x40,	0x20, 0x10,	0x08, 0x04,	0x02, 0x01 };

	// If the node doesn't exist, create it.
	if (*ppNode	== NULL)
		*ppNode	= (NODE*)CreateNode (nLevel, nColorBits, pLeafCount, pReducibleNodes);

	// Update color	information	if it's	a leaf node.
	if ((*ppNode)->bIsLeaf)	{
		(*ppNode)->nPixelCount++;
		(*ppNode)->nRedSum += r;
		(*ppNode)->nGreenSum +=	g;
		(*ppNode)->nBlueSum	+= b;
	} else {	// Recurse a level deeper if the node is not a leaf.
		int	shift =	7 -	nLevel;
		int	nIndex =(((r & mask[nLevel]) >> shift) << 2) |
			(((g & mask[nLevel]) >>	shift) << 1) |
			(( b & mask[nLevel]) >> shift);
		AddColor (&((*ppNode)->pChild[nIndex]),	r, g, b, nColorBits,
			nLevel + 1,	pLeafCount,	pReducibleNodes);
	}
}
/////////////////////////////////////////////////////////////////////////////
void* CQuantizer::CreateNode (UINT nLevel, UINT	nColorBits,	UINT* pLeafCount,
							  NODE** pReducibleNodes)
{
	NODE* pNode = (NODE*)calloc(1,sizeof(NODE));

	if (pNode== NULL) return NULL;

	pNode->bIsLeaf = (nLevel ==	nColorBits)	? TRUE : FALSE;
	if (pNode->bIsLeaf) (*pLeafCount)++;
	else {
		pNode->pNext = pReducibleNodes[nLevel];
		pReducibleNodes[nLevel]	= pNode;
	}
	return pNode;
}
/////////////////////////////////////////////////////////////////////////////
void CQuantizer::ReduceTree	(UINT nColorBits, UINT*	pLeafCount,
							 NODE** pReducibleNodes)
{
	int i;
	// Find	the	deepest	level containing at	least one reducible	node.
	for	(i=nColorBits -	1; (i>0) &&	(pReducibleNodes[i]	== NULL); i--);

	// Reduce the node most	recently added to the list at level	i.
	NODE* pNode	= pReducibleNodes[i];
	pReducibleNodes[i] = pNode->pNext;

	UINT nRedSum = 0;
	UINT nGreenSum = 0;
	UINT nBlueSum =	0;
//	UINT nAlphaSum = 0;
	UINT nChildren = 0;

	for	(i=0; i<8; i++)	
	{
		if (pNode->pChild[i] !=	NULL) 
		{
			nRedSum	+= pNode->pChild[i]->nRedSum;
			nGreenSum += pNode->pChild[i]->nGreenSum;
			nBlueSum +=	pNode->pChild[i]->nBlueSum;
//			nAlphaSum += pNode->pChild[i]->nAlphaSum;
			pNode->nPixelCount += pNode->pChild[i]->nPixelCount;
			free(pNode->pChild[i]);
			pNode->pChild[i] = NULL;
			nChildren++;
		}
	}

	pNode->bIsLeaf = TRUE;
	pNode->nRedSum = nRedSum;
	pNode->nGreenSum = nGreenSum;
	pNode->nBlueSum	= nBlueSum;
//	pNode->nAlphaSum	= nAlphaSum;
	*pLeafCount	-= (nChildren -	1);
}
/////////////////////////////////////////////////////////////////////////////
void CQuantizer::DeleteTree	(NODE**	ppNode)
{
	for	(int i=0; i<8; i++)	{
		if ((*ppNode)->pChild[i] !=	NULL) DeleteTree (&((*ppNode)->pChild[i]));
	}
	free(*ppNode);
	*ppNode	= NULL;
}
/////////////////////////////////////////////////////////////////////////////
void CQuantizer::GetPaletteColors (NODE* pTree,	BYTE* prgb, UINT* pIndex)
{
	if (pTree)
	{
		if (pTree->bIsLeaf)	
		{
			prgb[(*pIndex)+COLORTABLE_RED]   = (BYTE)std::min(255.f, 0.5f + (pTree->nRedSum)   /(float) pTree->nPixelCount);
			prgb[(*pIndex)+COLORTABLE_GREEN] = (BYTE)std::min(255.f, 0.5f + (pTree->nGreenSum) /(float) pTree->nPixelCount);
			prgb[(*pIndex)+COLORTABLE_BLUE]  = (BYTE)std::min(255.f, 0.5f + (pTree->nBlueSum)  /(float) pTree->nPixelCount);
//			prgb[*pIndex].rgbReserved =	(BYTE)min(255,0.5f + ((float)pTree->nAlphaSum) / pTree->nPixelCount);
// 			if (pSum) 
// 				pSum[*pIndex] = pTree->nPixelCount;
			(*pIndex)+=3;
		}
		else 
		{
			for	(int i=0; i<8; i++)	
			{
				if (pTree->pChild[i] !=	NULL)
					GetPaletteColors (pTree->pChild[i],	prgb, pIndex);
			}
		}
	}
}
/////////////////////////////////////////////////////////////////////////////
// UINT CQuantizer::GetColorCount ()
// {
// 	return m_nLeafCount;
// }
/////////////////////////////////////////////////////////////////////////////
void CQuantizer::SetColorTable (BYTE* prgb)
{
	UINT nIndex	= 0;
	if (m_nOutputMaxColors<16)
	{
		/*
		UINT nSum[16];
		RGBQUAD tmppal[16];
		GetPaletteColors (m_pTree, tmppal, &nIndex, nSum);
		if (m_nLeafCount>m_nOutputMaxColors) 
		{
			UINT j,k,nr,ng,nb,na,ns,a,b;
			for (j=0;j<m_nOutputMaxColors;j++)
			{
				a=(j*m_nLeafCount)/m_nOutputMaxColors;
				b=((j+1)*m_nLeafCount)/m_nOutputMaxColors;
				nr=ng=nb=na=ns=0;
				for (k=a;k<b;k++){
					nr+=tmppal[k].rgbRed * nSum[k];
					ng+=tmppal[k].rgbGreen * nSum[k];
					nb+=tmppal[k].rgbBlue * nSum[k];
//					na+=tmppal[k].rgbReserved * nSum[k];
					ns+= nSum[k];
				}
				prgb[j].rgbRed   = (BYTE)min(255,0.5f + ((float)nr)/ns);
				prgb[j].rgbGreen = (BYTE)min(255,0.5f + ((float)ng)/ns);
				prgb[j].rgbBlue  = (BYTE)min(255,0.5f + ((float)nb)/ns);
//				prgb[j].rgbReserved = (BYTE)min(255,0.5f + ((float)na)/ns);
			}
			
		} 
		else 
		{
			memcpy(prgb,tmppal,m_nLeafCount * sizeof(RGBQUAD));
		}
		*/
	} 
	else 
	{
		GetPaletteColors (m_pTree, prgb, &nIndex);
	}
}
/////////////////////////////////////////////////////////////////////////////
/*
BYTE CQuantizer::GetPixelIndex(long x, long y, int nbit, long effwdt, BYTE *pimage)
{
	if (nbit==8){
		return pimage[y*effwdt + x];
	} else {
		BYTE pos;
		BYTE iDst= pimage[y*effwdt + (x*nbit >> 3)];
		if (nbit==4){
			pos = (BYTE)(4*(1-x%2));
			iDst &= (0x0F<<pos);
			return (iDst >> pos);
		} else if (nbit==1){
			pos = (BYTE)(7-x%8);
			iDst &= (0x01<<pos);
			return (iDst >> pos);
		}
	}
	return 0;
}
*/