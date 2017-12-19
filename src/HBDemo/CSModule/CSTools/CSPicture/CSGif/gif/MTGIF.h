// ImageGif.h: interface for the CImageGif class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IMAGEGIF_H__0A00D358_C7E0_4C5C_AB60_515A04B0D9FC__INCLUDED_)
#define AFX_IMAGEGIF_H__0A00D358_C7E0_4C5C_AB60_515A04B0D9FC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "gifdefine.h"
#include "xiofile.h"
#include "Quantize.h"

#define TRANSPARENCY_CODE 0xF9
typedef short int code_int ;

//LZW GIF Image compression
#define MAXBITSCODES    12
#define HSIZE  5003     /* 80% occupancy */
#define MAXCODE(n_bits) (((code_int) 1 << (n_bits)) - 1)
//#define HashTabOf(i)    htab[i]
//#define CodeTabOf(i)    codetab[i]

#define EOFF -1


#define ntohs2(a) ( ((a) & 0xff) << 8 ) | ( ((a) >> 8) & 0xff )

#pragma pack(1)


typedef struct tag_gifgce
{
	BYTE flags; /*res:3|dispmeth:3|userinputflag:1|transpcolflag:1*/
	WORD delaytime;
	BYTE transpcolindex;
} struct_gifgce;

#pragma pack()

class CMTGIF
{
public:
	CMTGIF();
	virtual ~CMTGIF();
	
	bool WriteGifHeader(char* gifFilePath,int ncount,int w,int h,int time);
	
	bool SendGifFrame(BYTE *frameData);
	
	bool Close();

	bool DecreaseBpp2(DWORD nbit, bool errordiffusion, DWORD clrimportant,BYTE*pDest,BYTE*pSrcImageData,BYTE*pColorTable);
		
	void EncodeBody(CxFile *fp,BYTE* pImageData,BYTE*pData = NULL, bool bLocalColorMap = false) ;
	void EncodeHeader(CxFile *fp,BYTE* pData) ;
	void Putword(int w, CxFile *fp ) ;
	void EncodeLoopExtension(CxFile *fp) ;
	void EncodeExtension(CxFile *fp) ;
	void compressLZW( int init_bits,BYTE*pImageData, CxFile* outfile) ;

	BYTE GetNearestIndex2(BYTE* c,BYTE* pSrc);
	int GifNextPixel(BYTE* pImageData);
	void cl_hash(long hsize);
	void output( code_int  code);
	void char_out(int c);
	void flush_char();

protected:
	int m_nDelayTime;
	
	bool m_last_c_isvalid;
	BYTE m_last_c[3];
	BYTE m_last_c_index;
//	int m_loops ;		// 循环次数, -1表示无限
	struct_gifgce m_gifgce;

	int         m_curx, m_cury;
	unsigned long    m_cur_accum;
	int              m_cur_bits;

protected:
	long m_htab [HSIZE];
	unsigned short m_codetab [HSIZE];
	int m_n_bits;				/* number of bits/code */
	code_int m_maxcode;		/* maximum code, given n_bits */
	code_int m_free_ent;		/* first unused entry */
	int m_clear_flg;
	int m_init_bits;
	CxFile* m_outfile;
//	int ClearCode;
	int m_EOFCode;
	
	int m_a_count;
	char m_accum[256];
private:
	CxIOFile m_hFile;							  //保存文件地址
	BYTE* m_pImageData;
	CQuantizer* m_quantizer;	
	BYTE m_pColorTable[256*3];	//RGB
public:
	int m_nWidth;
	int m_nHeight;
	int m_nFrameCount;
};

extern CMTGIF* g_CMTGIF;

#endif // !defined(AFX_IMAGEGIF_H__0A00D358_C7E0_4C5C_AB60_515A04B0D9FC__INCLUDED_)
