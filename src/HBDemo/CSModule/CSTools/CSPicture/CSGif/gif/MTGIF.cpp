// ImageGif.cpp: implementation of the CImageGif class.
//
//////////////////////////////////////////////////////////////////////

#include "MTGIF.h"
#include "define.h"

#include <algorithm>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
//#define new DEBUG_NEW
#endif

CMTGIF* g_CMTGIF = NULL;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMTGIF::CMTGIF()
{
	m_pImageData = NULL;
	m_quantizer = NULL;
	
//	memset(m_comment,0,256);	
}

CMTGIF::~CMTGIF()
{

}

// write a word
void CMTGIF::Putword(int w, CxFile *fp )
{
	fp->PutC((BYTE)(w & 0xff));
	fp->PutC((BYTE)((w >> 8) & 0xff));
}

void CMTGIF::EncodeHeader(CxFile *fp,BYTE* pData)
{
//	Bitmap* g_bmp1 = new Bitmap(L"C:\\Users\\aidy\\Desktop\\新建文件夹\\01.jpg");
	fp->Write("GIF89a",1,6);	   //GIF Header
	
	Putword(m_nWidth,fp);	   //Logical screen descriptor
	Putword(m_nHeight,fp);
	
	BYTE Flags = 0x80;
	Flags |=(8 - 1) << 5;
	Flags |=(8 - 1);

	fp->PutC(Flags); //GIF "packed fields"		全局性数据
	fp->PutC(0);	 //GIF "BackGround"			背景色调色板索引
	fp->PutC(0);	 //GIF "pixel aspect ratio"	逻辑屏幕的长宽比

	// 写全局调色板
//	if (head.biClrUsed!=0)
	{			
		fp->Write(pData,sizeof(BYTE),768);
	}

}
 

// 应用程序控制块
void CMTGIF::EncodeLoopExtension(CxFile *fp)
{
	fp->PutC('!');		//byte  1  : 33 (hex 0x21) GIF Extension code
	fp->PutC(255);		//byte  2  : 255 (hex 0xFF) Application Extension Label
	fp->PutC(11);		//byte  3  : 11 (hex (0x0B) Length of Application Block (eleven bytes of data to follow)
	fp->Write("NETSCAPE2.0",11,1);
	fp->PutC(3);			//byte 15  : 3 (hex 0x03) Length of Data Sub-Block (three bytes of data to follow)
	fp->PutC(1);			//byte 16  : 1 (hex 0x01)
	Putword(0,fp);
//	Putword(m_loops,fp);	//bytes 17 to 18 : 0 to 65535, an unsigned integer in lo-hi byte format. 
							//This indicate the number of iterations the loop should be executed.
	fp->PutC(0);			//bytes 19       : 0 (hex 0x00) a Data Sub-block Terminator. 
}


// 图形控制扩展
void CMTGIF::EncodeExtension(CxFile *fp)
{
	// TRK BEGIN : transparency
	fp->PutC('!');
	fp->PutC(TRANSPARENCY_CODE);
	
	m_gifgce.flags = 0;
	int nBkgndIndex = -1;
	m_gifgce.flags |= ((nBkgndIndex != -1) ? 1 : 0);
	m_gifgce.flags |= ((2 & 0x7) << 2);

	//time
	m_gifgce.delaytime = m_nDelayTime;//(WORD)info.dwFrameDelay;
	m_gifgce.transpcolindex = (BYTE)nBkgndIndex;	   
	
	//Invert byte order in case we use a byte order arch, then set it back <AMSN>
//	gifgce.delaytime = ntohs(gifgce.delaytime);

	fp->PutC(sizeof(m_gifgce));	// struct_gifgce 结构体，4字节
	fp->Write(&m_gifgce, sizeof(m_gifgce), 1);
	m_gifgce.delaytime = ntohs2(m_gifgce.delaytime);
	
	fp->PutC(0);
	// TRK END
}


// Image Block
void CMTGIF::EncodeBody(CxFile *fp,BYTE* pImageData,BYTE*pData, bool bLocalColorMap)
{
	m_curx = 0;
	m_cury = 0;//m_nHeight - 1;	//because we read the image bottom to top
	
	fp->PutC(',');
	
	Putword(0,fp);
	Putword(0,fp);
	Putword(m_nWidth,fp);
	Putword(m_nHeight,fp);
	
	BYTE Flags=0x00; //non-interlaced (0x40 = interlaced) (0x80 = LocalColorMap)
	if (bLocalColorMap)	
	{ 
		Flags|=0x80; 
		Flags|=8-1; 
	}

	fp->PutC(Flags);
	
	if (bLocalColorMap)
	{
		fp->Write(pData,sizeof(BYTE),768);
	}
	
	int InitCodeSize = 8;//head.biBitCount <=1 ? 2 : head.biBitCount;
	
	// Write out the initial code size
	fp->PutC((BYTE)InitCodeSize);
	
	// Go and actually compress the data
	compressLZW(InitCodeSize+1,pImageData, fp);
	
	// Write out a Zero-length packet (to end the series)
	fp->PutC(0);
}

void CMTGIF::compressLZW( int init_bits,BYTE*pImageData, CxFile* outfile)
{
	long fcode;
	long c;
	long ent;
	long hshift;
	long disp;
	long i;
	
	// g_init_bits - initial number of bits
	// g_outfile   - pointer to output file
	m_init_bits = init_bits;
	m_outfile = outfile;
	
	// Set up the necessary values
	m_cur_accum = m_cur_bits = m_clear_flg = 0;
	m_maxcode = (short)MAXCODE(m_n_bits = m_init_bits);
	code_int maxmaxcode = (code_int)1 << MAXBITSCODES;
	
	int ClearCode = (1 << (init_bits - 1));
	m_EOFCode = ClearCode + 1;
	m_free_ent = (short)(ClearCode + 2);
	
	m_a_count=0;
	ent = GifNextPixel( pImageData );
	pImageData++;

	hshift = 0;
	for ( fcode = (long) HSIZE;  fcode < 65536; fcode *= 2)
	{
			++hshift;
	}
	hshift = 8 - hshift;                /* set hash code range bound */
	cl_hash((long)HSIZE);        /* clear hash table */
	output( (code_int)ClearCode );
	
	static int caa = 0;
	
	while ( (c = GifNextPixel(pImageData )) != EOFF )
	{    
		pImageData ++;
		
		fcode = (long) (((long) c << MAXBITSCODES) + ent);
		i = (((code_int)c << hshift) ^ ent);    /* xor hashing */
		  
		if (m_htab [i] == fcode ) {
			ent = m_codetab[i];
			continue;
		} else if ( (long)m_htab [i] < 0 )      /* empty slot */{
			//goto nomatch;
			
			output ( (code_int) ent );
			ent = c;
			if ( m_free_ent < maxmaxcode ) {  
				m_codetab[i] = m_free_ent++; /* code -> hashtable */
				m_htab [i] = fcode;
			} else {
				cl_hash((long)HSIZE);
				m_free_ent=(short)(ClearCode+2);
				m_clear_flg=1;
				output((code_int)ClearCode);
			}
		}
		else{
			disp = HSIZE - i;           /* secondary hash (after G. Knott) */
			if ( i == 0 )	disp = 1;
			bool isContinue = false;
			do{
				if ( (i -= disp) < 0 )	i += HSIZE;
				if ( m_htab [i] == fcode ) {	
					ent = m_codetab[i]; 
					//continue; 
					isContinue = true;
					break;
				}
			}while((long)m_htab [i] > 0);
			if(!isContinue){
				output ( (code_int) ent );
				ent = c;
				if ( m_free_ent < maxmaxcode ) {  
					m_codetab[i] = m_free_ent++; /* code -> hashtable */
					m_htab [i] = fcode;
				} else {
					cl_hash((long)HSIZE);
					m_free_ent=(short)(ClearCode+2);
					m_clear_flg=1;
					output((code_int)ClearCode);
				}
			}
		}
	}//end of while
	
	// Put out the final code.
	output( (code_int)ent );
	output( (code_int) m_EOFCode );
}

////////////////////////////////////////////////////////////////////////////////
// Return the next pixel from the image
// <DP> fix for 1 & 4 bpp images
int CMTGIF::GifNextPixel(BYTE* pImageData)
{
 	if( m_cury == m_nHeight) 
		return EOFF;
// 	--CountDown;

	int r= pImageData[0];//GetPixelIndex(curx,cury);
 	// Bump the current X position
 	++m_curx;
 	if( m_curx == m_nWidth ){
 		m_curx = 0;
 		m_cury++;	             //bottom to top
 	}
	return r;
}
BYTE CMTGIF::GetNearestIndex2(BYTE* c,BYTE* pSrc)
{
	
	// <RJ> check matching with the previous result
	if (m_last_c_isvalid && m_last_c[COLORTABLE_RED] == c[MT_RED] 
		&& m_last_c[COLORTABLE_GREEN] == c[MT_GREEN] && 
		m_last_c[COLORTABLE_BLUE] == c[MT_BLUE]) 
		return m_last_c_index;
	//	last_c = c;
	m_last_c[COLORTABLE_RED] = c[MT_RED];
	m_last_c[COLORTABLE_GREEN] = c[MT_GREEN];
	m_last_c[COLORTABLE_BLUE] = c[MT_BLUE];
//	memcpy(last_c,c,sizeof(BYTE)*3);
	
	m_last_c_isvalid = true;
	
	BYTE* iDst = pSrc;//(BYTE*)(pDib) + sizeof(BITMAPINFOHEADER);
	long distance=200000;
	int i,j = 0;
	long k,l;
    
	for(i=0,l=0;i<256;i++)
	{
 		k = (iDst[l]-c[MT_RED])*(iDst[l]-c[MT_RED])+
 			(iDst[l+1]-c[MT_GREEN])*(iDst[l+1]-c[MT_GREEN])+
 			(iDst[l+2]-c[MT_BLUE])*(iDst[l+2]-c[MT_BLUE]);

		l+=3;//sizeof(RGBQUAD);
		if (k==0){
			j=i;
			break;
		}
		if (k<distance){
			distance=k;
			j=i;
		}
	}
	m_last_c_index = j;
	return j;
	
}
void CMTGIF::cl_hash(long hsize)
{
	long *htab_p = m_htab+hsize;
	
	long i;
	long m1 = -1L;
	
	i = hsize - 16;
	
	do {
		*(htab_p-16)=m1;
		*(htab_p-15)=m1;
		*(htab_p-14)=m1;
		*(htab_p-13)=m1;
		*(htab_p-12)=m1;
		*(htab_p-11)=m1;
		*(htab_p-10)=m1;
		*(htab_p-9)=m1;
		*(htab_p-8)=m1;
		*(htab_p-7)=m1;
		*(htab_p-6)=m1;
		*(htab_p-5)=m1;
		*(htab_p-4)=m1;
		*(htab_p-3)=m1;
		*(htab_p-2)=m1;
		*(htab_p-1)=m1;
		
		htab_p-=16;
	} while ((i-=16) >=0);
	
	for (i+=16;i>0;--i)
		*--htab_p=m1;
}

////////////////////////////////////////////////////////////////////////////////



void CMTGIF::output( code_int  code)
{
	const unsigned long code_mask[] = { 0x0000, 0x0001, 0x0003, 0x0007, 0x000F,
									0x001F, 0x003F, 0x007F, 0x00FF,
									0x01FF, 0x03FF, 0x07FF, 0x0FFF,
									0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF };
	m_cur_accum &= code_mask[ m_cur_bits ];
	

	if( m_cur_bits > 0 )
		m_cur_accum |= ((long)code << m_cur_bits);
	else
		m_cur_accum = code;
	
	m_cur_bits += m_n_bits;
	
	while( m_cur_bits >= 8 ) {
		char_out( (unsigned int)(m_cur_accum & 0xff) );
		m_cur_accum >>= 8;
		m_cur_bits -= 8;
	}
	
	/*
	* If the next entry is going to be too big for the code size,
	* then increase it, if possible.
	*/
	if ( m_free_ent > m_maxcode || m_clear_flg ) {
		if( m_clear_flg ) {
			m_maxcode = (short)MAXCODE(m_n_bits = m_init_bits);
			m_clear_flg = 0;
		} else {
			++m_n_bits;
			if ( m_n_bits == MAXBITSCODES )
				m_maxcode = (code_int)1 << MAXBITSCODES; /* should NEVER generate this code */
			else
				m_maxcode = (short)MAXCODE(m_n_bits);
		}
	}
	
	if( code == m_EOFCode ) {
		// At EOF, write the rest of the buffer.
		while( m_cur_bits > 0 ) {
			char_out( (unsigned int)(m_cur_accum & 0xff) );
			m_cur_accum >>= 8;
			m_cur_bits -= 8;
		}
		
		flush_char();
		m_outfile->Flush();
	}
}

void CMTGIF::char_out(int c)
{
	m_accum[m_a_count++]=(char)c;
	if (m_a_count >=254)
		flush_char();
}

void CMTGIF::flush_char()
{
	if (m_a_count > 0) {
		m_outfile->PutC((BYTE)m_a_count);
		m_outfile->Write(m_accum,1,m_a_count);
		m_a_count=0;
	}
}

bool CMTGIF::DecreaseBpp2(DWORD nbit, bool errordiffusion, DWORD clrimportant,BYTE*pDest,BYTE*pSrcImageData,BYTE*pColorTable)
{
	int er,eg,eb;
	m_last_c_isvalid = false;//20111205
	BYTE *pDestData = pDest;
	int width4 = m_nWidth<<2;
	for (long y=0;y<m_nHeight;y++)
	{
		BYTE*pp = pSrcImageData + (y)*width4;
		for (long x=0;x<m_nWidth;x++)
		{
				BYTE a2 = GetNearestIndex2(pp,pColorTable);
				pDestData[0] = a2;
				pDestData++;

				er=pp[MT_RED] - pColorTable[a2*3+COLORTABLE_RED];//(long)ce.rgbRed;
				eg=pp[MT_GREEN] - pColorTable[a2*3+COLORTABLE_GREEN];//(long)ce.rgbGreen;
				eb=pp[MT_BLUE] - pColorTable[a2*3+COLORTABLE_BLUE];//(long)ce.rgbBlue;

				if (x!=m_nWidth-1)
				{
					pp += 4;
					pp[MT_RED] = (BYTE)std::min(255,std::max(0,pp[MT_RED] + ((er*7) >> 4)));
					pp[MT_GREEN] = (BYTE)std::min(255,std::max(0,pp[MT_GREEN] + ((eg*7) >> 4)));
					pp[MT_BLUE] = (BYTE)std::min(255,std::max(0,pp[MT_BLUE] + ((eb*7) >> 4)));
					pp -= 4;
				}

				int coeff=1;

				pp += width4;
				for(int i=-1; i<2; i++)
				{
					switch(i)
					{
					case -1:
						coeff=2; break;
					case 0:
						coeff=4; break;
					case 1:
						coeff=1; break;
					}
					if (x+i<m_nWidth && x+i>=0  && (y+1)>=0 &&(y+1)<m_nHeight)
					{
						BYTE*pp2 = pp + (i<<2);
						
						pp2[MT_RED] = (BYTE)std::min(255,std::max(0,pp2[MT_RED] + ((er * coeff) >> 4)));
						pp2[MT_GREEN] = (BYTE)std::min(255,std::max(0,pp2[MT_GREEN] + ((eg * coeff) >> 4)));
						pp2[MT_BLUE] = (BYTE)std::min(255,std::max(0,pp2[MT_BLUE] + ((eb * coeff) >> 4)));
					}
				}
				pp = pp - width4 + 4;
			
		}
	}
	return true;
}

bool CMTGIF::WriteGifHeader(char* gifFilePath,int ncount,int w,int h,int time)
{
	int ColorCount=256;
	m_nFrameCount = ncount;
	m_nWidth = w;
	m_nHeight = h;
	//编码方式选择
	m_hFile.Open(gifFilePath,(char*)"wb+");//测试用的地址

	m_nDelayTime = time;
	
	EncodeHeader(&m_hFile,m_pColorTable);
	EncodeLoopExtension(&m_hFile);

	if(m_pImageData != NULL) {
        delete []m_pImageData;
        m_pImageData = NULL;
	}

	m_pImageData = new BYTE[m_nWidth*m_nHeight];
	if(m_quantizer != NULL) {
        delete m_quantizer;
        m_quantizer = NULL;
    }

	m_quantizer = new CQuantizer(ColorCount,8);
	m_quantizer->SetColorTable(m_pColorTable);
	return true;
}

bool CMTGIF::SendGifFrame(BYTE* frameData)
{
	int ColorCount=256;

	BYTE*pData = frameData;

	m_quantizer->ProcessImage2(pData,m_nWidth,m_nHeight);
	m_quantizer->SetColorTable(m_pColorTable);
	   
	DecreaseBpp2(8,1,ColorCount,m_pImageData,pData,m_pColorTable);			

	EncodeExtension(&m_hFile);
	EncodeBody(&m_hFile,m_pImageData,m_pColorTable,true);

	return true;
}


bool CMTGIF::Close()
{
	m_hFile.PutC(';'); // Write the GIF file terminator
	m_hFile.Close();
	if(m_pImageData != NULL){
		delete []m_pImageData;
		m_pImageData = NULL;
	}
	if(m_quantizer != NULL){
		delete m_quantizer;
		m_quantizer = NULL;
	}
	return true;
}
