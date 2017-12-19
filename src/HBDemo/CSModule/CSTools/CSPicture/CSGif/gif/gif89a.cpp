#include <string.h>
#include "gif89a.h"


CGif89a* g_gifReader;

CGif89a::CGif89a()
{	
	m_opened = FALSE;
	m_error = FALSE;


	m_strOpenPath = NULL;
}

CGif89a::CGif89a(char* fileName,BOOL inMem)
{	
	int len = strlen(fileName);

	m_strOpenPath = new char[len+1];
	strcpy(m_strOpenPath,fileName);
	m_strOpenPath[len] = '\0';

	if(open(fileName,inMem))
	{	m_opened = TRUE;
		m_error = FALSE;
	}
	else
	{	m_opened = FALSE;
		m_error = TRUE;
	}

}

CGif89a::~CGif89a()
{	
	close();
	if ( m_strOpenPath != NULL)
	{
		delete m_strOpenPath;
		m_strOpenPath = NULL;
	}
}

BOOL CGif89a::operator!()
{	
	return m_error;
}

BOOL CGif89a::open(char* fileName,BOOL b)
{	
	char cc[4];
	BYTE be;
	BOOL fileEnd = FALSE;
	m_inMem = b;
	m_allFrames = NULL;
	m_curIndex = 0;
	m_curFrame.pColorTable = NULL;
	m_curFrame.dataBuf = NULL;
	m_ctrlExt.active = FALSE;

	m_gifPath = fileName;
	m_file = fopen(fileName,"rb");
//	FILE *pFile  = fopen(m_gifPath, "rb"); //打开文件
//	m_file.open(fileName,ios::binary);
	if(!m_file)
		return false;
	fread(cc,sizeof(char),3,m_file);

//	m_file.read(cc,3);
	if(strncmp(cc,"GIF",3) != 0){
		//fclose(pFile);
		fclose(m_file);
		return false;
	}

//	fread(version,sizeof(char),3,pFile);
//	m_file.read(m_version,3);
	fread((char*)m_version,sizeof(char),3,m_file);
	m_version[3] = 0;
	if(strncmp(m_version,"89a",3) > 0){
		//fclose(pFile);
		fclose(m_file);
		return false;
	}
		

	fread((char*)&m_gifInfo.scrWidth,sizeof(char),2,m_file);
	fread((char*)&m_gifInfo.scrHeight,sizeof(char),2,m_file);
// 	m_file.read((char*)&m_gifInfo.scrWidth,2);
// 	m_file.read((char*)&m_gifInfo.scrHeight,2);

//	fread((char*)&be,sizeof(char),1,pFile);
//	m_file.read((char*)&be,1);
	fread((char*)&be,sizeof(char),1,m_file);
	if((be&0x80) != 0)
		m_gifInfo.gFlag = TRUE;
	else
		m_gifInfo.gFlag = FALSE;
	m_gifInfo.colorRes = ((be&0x70)>>4)+1;
	if(m_gifInfo.gFlag)
	{	if((be&0x08) != 0)
			m_gifInfo.gSort = TRUE;
		else
			m_gifInfo.gSort = FALSE;
		m_gifInfo.gSize = 1;
		m_gifInfo.gSize <<= ((be&0x07)+1);
	}
	fread((char*)&be,sizeof(char),1,m_file);
//	fread((char*)&be,sizeof(char),1,pFile);
	m_gifInfo.BKColorIdx = be;
	fread((char*)&be,sizeof(char),1,m_file);
//	fread((char*)&be,sizeof(char),1,pFile);
	m_gifInfo.pixelAspectRatio = be;

	///////////////////////////////////////
	if(m_gifInfo.gFlag)
	{	
		fread((char*)m_bColorTable,sizeof(char),m_gifInfo.gSize*3,m_file);
		//fread((char*)gColorTable,sizeof(char),gInfo.gSize*3,pFile);
		m_gifInfo.gColorTable = m_bColorTable;
	}
	else
		m_gifInfo.gColorTable = NULL;
//	m_dataStart = m_file.tellg();
	m_dataStart = ftell(m_file);
	if((m_gifInfo.frames = checkFrames()) == 0){
		//fclose(pFile);
		fclose(m_file);
		return false;
	}
	if(m_inMem)
	{	
		if((m_allFrames = new FRAME[m_gifInfo.frames]) == NULL){
			fclose(m_file);
			return false;
		}
		//ZeroMemory(allFrames,sizeof(FRAME)*gInfo.frames);
		//读取每一帧的信息
		memset(m_allFrames,0,sizeof(FRAME)*m_gifInfo.frames);
		if(!getAllFrames())
		{	
			delete[] m_allFrames;
			m_allFrames = NULL;
			//fclose(pFile);
			fclose(m_file);
			return false;
		}
		fclose(m_file);
//		fclose(pFile);
	}
	m_opened = TRUE;
	return TRUE;
}

UINT CGif89a::checkFrames()
{	
	BYTE be;
	BOOL fileEnd = FALSE;
	UINT frames=0;
//	streampos pos = m_file.tellg();
	int pos = ftell(m_file);
	while(!feof(m_file) && !fileEnd)
	{	
		fread((char*)&be,sizeof(char),1,m_file);
		switch(be)
		{	case 0x21:
				fread((char*)&be,sizeof(char),1,m_file);
				switch(be)
				{	case 0xf9:
					case 0xfe:
					case 0x01:
					case 0xff:
						while(!feof(m_file))
						{	fread((char*)&be,sizeof(char),1,m_file);
							if(be == 0)
								break;
							fseek(m_file,be,SEEK_CUR);
						}
						break;
					default:
						return 0;
				}
				break;
			case 0x2c:
			{	BYTE bp;
				BOOL lFlag=FALSE;
				UINT lSize=1;
				frames++;

				fseek(m_file,8,SEEK_CUR);
				fread((char*)&bp,sizeof(char),1,m_file);
				if((bp&0x80) != 0)
					lFlag = TRUE;
				lSize <<= ((bp&0x07)+1);
				if(lFlag)
					fseek(m_file,lSize*3,SEEK_CUR);

				fread((char*)&be,sizeof(char),1,m_file);
				while(!feof(m_file))
				{	fread((char*)&be,sizeof(char),1,m_file);
					if(be == 0)
						break;
					fseek(m_file,be,SEEK_CUR);
				}
				break;
			}
			case 0x3b:
				fileEnd = TRUE;
				break;
			case 0x00:
				break;
			default:
				return 0;
		}
	}
	fseek(m_file,pos,SEEK_SET);
	return frames;
}

BOOL CGif89a::getAllFrames()
{	
	BYTE be;
	BOOL fileEnd = FALSE;
	FRAME *pf = m_allFrames;
//	streampos pos = ifs.tellg();
	int pos = ftell(m_file);
	int i;
	while(!feof(m_file) && !fileEnd)
	{	fread((char*)&be,sizeof(char),1,m_file);
		switch(be)
		{	case 0x21:
				fread((char*)&be,sizeof(char),1,m_file);
				switch(be)
				{	case 0xf9:
						while(!feof(m_file))
						{	fread((char*)&be,sizeof(char),1,m_file);
							if(be == 0)
								break;
							if(be == 4)
							{	m_ctrlExt.active = TRUE;
								fread((char*)&be,sizeof(char),1,m_file);
								m_ctrlExt.disposalMethod = (be&0x1c)>>2;
								if((be&0x02) != 0)
									m_ctrlExt.userInputFlag = TRUE;
								else
									m_ctrlExt.userInputFlag = FALSE;
								if((be&0x01) != 0)
									m_ctrlExt.trsFlag = TRUE;
								else
									m_ctrlExt.trsFlag = FALSE;
								fread((char*)&m_ctrlExt.delayTime,sizeof(char),2,m_file);
								fread((char*)&be,sizeof(char),1,m_file);
								m_ctrlExt.trsColorIndex = be;
							}
							else
								fseek(m_file,be,SEEK_CUR);
						}
						break;
					case 0xfe:
					case 0x01:
					case 0xff:
						while(!feof(m_file))
						{	fread((char*)&be,sizeof(char),1,m_file);
							if(be == 0)
								break;
							fseek(m_file,be,SEEK_CUR);
						}
						break;
					default:
						goto error;
				}
				break;
			case 0x2c:
			{	BYTE bp;
				fread((char*)&pf->imageLPos,sizeof(char),2,m_file);
				fread((char*)&pf->imageTPos,sizeof(char),2,m_file);
				fread((char*)&pf->imageWidth,sizeof(char),2,m_file);
				fread((char*)&pf->imageHeight,sizeof(char),2,m_file);
				fread((char*)&bp,sizeof(char),1,m_file);
				if((bp&0x80) != 0)
					pf->lFlag = TRUE;
				if((bp&0x40) != 0)
					pf->interlaceFlag = TRUE;
				if((bp&0x20) != 0)
					pf->sortFlag = TRUE;
				pf->lSize = 1;
				pf->lSize <<= ((bp&0x07)+1);
				if(pf->lFlag)
				{	if((pf->pColorTable = new BYTE[pf->lSize*3]) == NULL)
						goto error;

					fread((char*)pf->pColorTable,sizeof(char),pf->lSize*3,m_file);
				}

				if(!extractData(pf))
					goto error;
				if(m_ctrlExt.active)
				{	pf->ctrlExt = m_ctrlExt;
					m_ctrlExt.active = FALSE;
				}
				pf++;
				break;
			}
			case 0x3b:
				fileEnd = TRUE;
				break;
			case 0x00:
				break;
			default:
				goto error;
		}
	}
	fseek(m_file,pos,SEEK_SET);
//	ifs.seekg(pos);
	return TRUE;
error:
	pf = m_allFrames;
	for(i=0;i<(int)m_gifInfo.frames;i++)
	{	if(pf->pColorTable != NULL)
		{	delete[] pf->pColorTable;
			pf->pColorTable = NULL;
		}
		if(pf->dataBuf != NULL)
		{	delete[] pf->dataBuf;
			pf->dataBuf = NULL;
		}
	}
	return FALSE;
}

BYTE* CGif89a::getNextFrameT(int *deley)
{
	LOGGIF("getNextFrameT__0");
	LPCFRAME lpf = getNextFrame();
	LOGGIF("getNextFrameT");
	if(lpf != NULL){
		LOGGIF("getNextFrameT_______");
		*deley = lpf->ctrlExt.delayTime;
		LOGGIF("getNextFrameT success *deley=%d",*deley);
		int x,y;
		BYTE* pData = lpf->dataBuf;
		BYTE* pColorTable = lpf->pColorTable;
		
		int nWidth = lpf->imageWidth;
		int nHeight= lpf->imageHeight;
		BYTE* pInputDst = new BYTE[nWidth*nHeight*4];
		BYTE* pInput = pInputDst;
		for (y=0;y<nHeight;y++)
		{
			for (x=0;x<nWidth;x++)
			{
				pInput[MT_ALPHA] = 255;
				pInput[MT_RED] = pColorTable[pData[0]*3+0];
				pInput[MT_GREEN] = pColorTable[pData[0]*3+1];
				pInput[MT_BLUE] = pColorTable[pData[0]*3+2];

				pInput += 4;
				pData +=1;
			}
		}
//		LOGGIF("getNextFrameT3");
		delete []lpf->dataBuf;
		lpf->dataBuf = NULL;
		delete []lpf->pColorTable;
		lpf->pColorTable = NULL;
//		LOGGIF("getNextFrameT4");
		return pInputDst;
	}
	return NULL;
}

LPCFRAME CGif89a::getNextFrame()
{	
//	LOGGIF("getNextFrame");
	if(m_inMem)
	{	
		FRAME* p =  m_allFrames+m_curIndex;
		m_curIndex++;
		if(m_curIndex >= m_gifInfo.frames)
			m_curIndex = 0;
		return p;
	}
	else
	{	
//		LOGGIF("getNextFrame  else");
		BYTE be;
		BOOL fileEnd = FALSE;
//		LOGGIF("getNextFrame  ___0");
		if(m_curFrame.pColorTable != NULL)
		{	delete[] m_curFrame.pColorTable;
			m_curFrame.pColorTable = NULL;
		}
//		LOGGIF("getNextFrame  ___01");
		if(m_curFrame.dataBuf != NULL)
		{	delete[] m_curFrame.dataBuf;
			m_curFrame.dataBuf = NULL;
		}
		//ZeroMemory(&curFrame,sizeof(FRAME));
		memset(&m_curFrame,0,sizeof(FRAME));
		while(TRUE)
		{
	
			fread((char*)&be,sizeof(char),1,m_file);
			LOGGIF("getNextFrame  ___12 be=%d",be);
			switch(be)
			{	case 0x21:
//					LOGGIF("getNextFrame  ___13");
					fread((char*)&be,sizeof(char),1,m_file);
//					LOGGIF("getNextFrame  ___131 be=%d",be);
					switch(be)
					{	case 0xf9:
//							LOGGIF("getNextFrame  ___131");
							while(!feof(m_file))
							{	fread((char*)&be,sizeof(char),1,m_file);
								if(be == 0)
									break;
								if(be == 4)
								{	m_ctrlExt.active = TRUE;
									fread((char*)&be,sizeof(char),1,m_file);
									m_ctrlExt.disposalMethod = (be&0x1c)>>2;
									if((be&0x02) != 0)
										m_ctrlExt.userInputFlag = TRUE;
									else
										m_ctrlExt.userInputFlag = FALSE;
									if((be&0x01) != 0)
										m_ctrlExt.trsFlag = TRUE;
									else
										m_ctrlExt.trsFlag = FALSE;
									fread((char*)&m_ctrlExt.delayTime,sizeof(char),2,m_file);
									fread((char*)&be,sizeof(char),1,m_file);
									m_ctrlExt.trsColorIndex = be;
								}
								else
									fseek(m_file,be,SEEK_CUR);
							}
							break;
						case 0xfe:
						case 0x01:
						case 0xff:
//							LOGGIF("getNextFrame  ___14");
							while(!feof(m_file))
							{	fread((char*)&be,sizeof(char),1,m_file);
								if(be == 0)
									break;
//								m_file.seekg(be,ios::cur);
								fseek(m_file,be,SEEK_CUR);
							}
							break;
						default:
							goto error;
					}
					break;
				case 0x2c:
				{	
//					LOGGIF("getNextFrame  ___15");
					BYTE bp;
					fread((char*)&m_curFrame.imageLPos,sizeof(char),2,m_file);
					fread((char*)&m_curFrame.imageTPos,sizeof(char),2,m_file);
					fread((char*)&m_curFrame.imageWidth,sizeof(char),2,m_file);
					fread((char*)&m_curFrame.imageHeight,sizeof(char),2,m_file);
					fread((char*)&bp,sizeof(char),1,m_file);
//					LOGGIF("getNextFrame  ___151");
					if((bp&0x80) != 0)
						m_curFrame.lFlag = TRUE;
					if((bp&0x40) != 0)
						m_curFrame.interlaceFlag = TRUE;
					if((bp&0x20) != 0)
						m_curFrame.sortFlag = TRUE;
//					LOGGIF("getNextFrame  ___152");
					m_curFrame.lSize = 1;
					m_curFrame.lSize <<= ((bp&0x07)+1);
//					LOGGIF("getNextFrame  ___1522");
					if((m_curFrame.pColorTable = new BYTE[m_curFrame.lSize*3]) == NULL)
						goto error;
//					LOGGIF("getNextFrame  ___1523");
					if(m_curFrame.lFlag == TRUE){
						fread((char*)m_curFrame.pColorTable,sizeof(char),m_curFrame.lSize*3,m_file);
					}
					else if (m_allFrames != NULL && m_allFrames->pColorTable != NULL)
					{
//						LOGGIF("getNextFrame  ___1525");
						memcpy(m_curFrame.pColorTable,m_allFrames->pColorTable,m_curFrame.lSize*3);
					}
//					LOGGIF("getNextFrame  ___153");
					int rr = m_curFrame.pColorTable[360];
					int gg = m_curFrame.pColorTable[361];

					if(!extractData(&m_curFrame)){
//						LOGGIF("getNextFrame  ___155");
						goto error;
					}
					m_curFrame.ctrlExt = m_ctrlExt;
					if(m_ctrlExt.active = TRUE)
						m_ctrlExt.active = FALSE;
//					LOGGIF("getNextFrame  ___156");
					return &m_curFrame;
				}
				case 0x3b:
//					LOGGIF("getNextFrame  ___16");
					fseek(m_file,m_dataStart,SEEK_SET);
					break;
				case 0x00:
//					LOGGIF("getNextFrame  ___161");
					break;
				default:
//					LOGGIF("getNextFrame  ___17");
					goto error;
			}
		}	
//		LOGGIF("getNextFrame  ___9");
		return &m_curFrame;
error:
//		LOGGIF("getNextFrame  ___10");
		if(m_curFrame.pColorTable != NULL)
		{	delete[] m_curFrame.pColorTable;
			m_curFrame.pColorTable = NULL;
		}
		if(m_curFrame.dataBuf != NULL)
		{	delete[] m_curFrame.dataBuf;
			m_curFrame.dataBuf = NULL;
		}
		return NULL;
	}
}

BOOL CGif89a::initStrTable(STRING_TABLE_ENTRY* strTable,UINT rootSize)
{	UINT i;
	unsigned char *cc;
	for(i=0;i<rootSize;i++)
	{	if((cc = new unsigned char[2]) == NULL)
			goto error;
		cc[0] = i,cc[1] = 0;
		strTable[i].p = cc;
		strTable[i].len = 1;
	}
	return TRUE;
error:
	for(i=0;i<rootSize;i++)
		if(strTable[i].p != NULL)
		{	delete[] strTable[i].p;
			strTable[i].p = NULL;
		}
	return FALSE;
}

BOOL CGif89a::addStrTable(STRING_TABLE_ENTRY* strTable,UINT addIdx,UINT idx,unsigned char c)
{	unsigned char *cc;
	UINT l = strTable[idx].len;
	if(addIdx >= 4096)
		return FALSE;
	if((cc = new unsigned char[l+2]) == NULL)
		return FALSE;
	for(UINT i=0;i<l;i++)
		cc[i] = strTable[idx].p[i];
	cc[l] = c;
	cc[l+1] = 0;
	strTable[addIdx].p = cc;
	strTable[addIdx].len = strTable[idx].len +1;
	return TRUE;
}

BOOL CGif89a::extractData(FRAME* f)
{	
	//LOGGIF("extractData  ___1  w=%d h=%d",f->imageWidth,f->imageHeight);
	STRING_TABLE_ENTRY *strTable;
	UINT codeSize,rootSize,tableIndex,codeSizeBK;
	int remainInBuf = 0,i;
	UINT bufIndex = 0,outIndex = 0;
	UINT bitIndex = 0;
	DWORD code,oldCode;
	BYTE be,*outP;
	BYTE buf[262];
	BOOL readOK = FALSE;
	UINT bufLen = f->imageWidth*f->imageHeight;
	if((strTable = new STRING_TABLE_ENTRY[4096]) == NULL){
		//LOGGIF("extractData  ___21");
		return FALSE;
	}
	//ZeroMemory(strTable,sizeof(STRING_TABLE_ENTRY)*4096);
	memset(strTable,0,sizeof(STRING_TABLE_ENTRY)*4096);
	outP = f->dataBuf = new BYTE[bufLen];
	//LOGGIF("extractData  ___3");
	if(f->dataBuf == NULL)
	{	
		//LOGGIF("extractData  ___31");
		delete[] strTable;
		return FALSE;
	}
	fread((char*)&be,sizeof(char),1,m_file);
	codeSizeBK = codeSize = be+1;
	rootSize = 1;
	rootSize <<= be; 
	tableIndex = rootSize+2;
	//LOGGIF("extractData  ___4 bufIndex=%d bitIndex=%d",bufIndex,bitIndex);
	if(!initStrTable(strTable,rootSize)){
		//LOGGIF("extractData  ___41");
		goto error;
	}

begin:
	//LOGGIF("extractData  ___5 remainInBuf=%d readOK=%d",remainInBuf,readOK);
	if(remainInBuf<=4 && !readOK)
	{	for(i=0;i<remainInBuf;i++)
			buf[i] = buf[bufIndex+i];
		bufIndex = 0;
		fread((char*)&be,sizeof(char),1,m_file);
		if(be != 0)
		{
			fread((char*)(buf+remainInBuf),sizeof(char),be,m_file);
			remainInBuf += be;
		}
		else
			readOK = TRUE;

	}
	//LOGGIF("extractData  ___6 bufIndex=%d buf=%d",bufIndex,buf[bufIndex]);
	if(remainInBuf<=4)
		if(remainInBuf<=0 || codeSize > (remainInBuf*8-bitIndex)){
			//LOGGIF("extractData  ___61");
			goto done;
		}
	//LOGGIF("extractData  ___codeSize=%d bitIndex=%d",codeSize,bitIndex);
	code = *((DWORD*)(buf+bufIndex));
	//LOGGIF("extractData  ___code1=%d",code);
	code <<= 32-codeSize-bitIndex;
	code >>= 32-codeSize;
	bitIndex += codeSize;
	bufIndex += bitIndex/8;
	remainInBuf -= bitIndex/8;
	bitIndex %= 8;
	//LOGGIF("extractData  ___rootSize=%d code=%d",rootSize,code);
	if(code >= rootSize+1){
		//LOGGIF("extractData  ___71");
		goto error;
	}
	if(code == rootSize){
		//LOGGIF("extractData  ___72");
		goto begin;
	}
	else
	{	outP[outIndex++] = *strTable[code].p;
		oldCode = code;
	}
	//LOGGIF("extractData  ___8");
	for(;;)
	{	
		if(remainInBuf<=4 && !readOK)
		{	
			for(i=0;i<remainInBuf;i++)
				buf[i] = buf[bufIndex+i];
			bufIndex = 0;
			fread((char*)&be,sizeof(char),1,m_file);
			if(be != 0)
			{	
				fread((char*)(buf+remainInBuf),sizeof(char),be,m_file);
				remainInBuf += be;
			}
			else
				readOK = TRUE;

		}
		if(remainInBuf<=4)
			if(remainInBuf<=0 || codeSize > (remainInBuf*8-bitIndex))
				break;
		code = *((DWORD*)(buf+bufIndex));
		code <<= 32-codeSize-bitIndex;
		code >>= 32-codeSize;
		bitIndex += codeSize;
		bufIndex += bitIndex/8;
		remainInBuf -= bitIndex/8;
		bitIndex %= 8;
		if(code == rootSize)
		{	codeSize = codeSizeBK;
			for(i=rootSize;i<4096;i++)
				if(strTable[i].p != NULL)
				{	delete strTable[i].p;
					strTable[i].p = NULL;
					strTable[i].len = 0;
				}
			tableIndex = rootSize+2;
			goto begin;
		}
		else if(code == rootSize+1)
			break;
		else
		{	unsigned char *p = strTable[code].p;
			int l = strTable[code].len;
			unsigned char c;
			if(p != NULL)
			{	c = *p;
				if(outIndex+l <= bufLen)
					for(i=0;i<l;i++)
						outP[outIndex++] = *p++;
				else
					goto error;
				if(!addStrTable(strTable,tableIndex++,oldCode,c))
					goto error;
				oldCode = code;
			}
			else
			{	p = strTable[oldCode].p;
				l = strTable[oldCode].len;
				c = *p;
				if(outIndex+l+1 <= bufLen)
				{	for(i=0;i<l;i++)
						outP[outIndex++] = *p++;
					outP[outIndex++] = c;
				}
				else
					goto error;
				if(!addStrTable(strTable,tableIndex++,oldCode,c))
					goto error;
				oldCode = code;
			}
			if(tableIndex == (((UINT)1)<<codeSize) && codeSize != 12)
				codeSize++;
		}
	}
	//LOGGIF("extractData  ___10");
done:
	for(i=0;i<4096;i++)
		if(strTable[i].p != NULL)
		{	delete strTable[i].p;
			strTable[i].p = NULL;
		}
	delete[] strTable;
//	LOGGIF("extractData  ___101");
	return TRUE;
error:
	//LOGGIF("extractData  ___111");
	for(i=0;i<4096;i++)
		if(strTable[i].p != NULL)
		{	delete strTable[i].p;
			strTable[i].p = NULL;
		}
	delete[] strTable;
	delete[] f->dataBuf;
	f->dataBuf = NULL;
	return FALSE;
}

LPCGLOBAL_INFO CGif89a::getGlobalInfo()
{	return &m_gifInfo;
}

void CGif89a::close()
{	if(m_opened)
	{	m_opened = FALSE;
		if(m_inMem && m_allFrames != NULL)
		{	FRAME* pf = m_allFrames;
			for(UINT i=0;i<m_gifInfo.frames;i++)
			{	if(pf->pColorTable != NULL)
				{	delete[] pf->pColorTable;
					pf->pColorTable = NULL;
				}
				if(pf->dataBuf != NULL)
				{	delete[] pf->dataBuf;
					pf->dataBuf = NULL;
				}
			}
			delete[] m_allFrames;
			m_allFrames = NULL;
		}
		if(!m_inMem)
		{	if(m_curFrame.pColorTable != NULL)
			{	delete[] m_curFrame.pColorTable;
				m_curFrame.pColorTable = NULL;
			}
			if(m_curFrame.dataBuf != NULL)
			{	delete[] m_curFrame.dataBuf;
				m_curFrame.dataBuf = NULL;
			}
			fclose(m_file);
		}
	}
}

LPCSTR CGif89a::getVer()
{	return m_version;
}

int CGif89a::getWidth()
{
	return m_gifInfo.scrWidth;
}
int CGif89a::getHeight()
{
	return m_gifInfo.scrHeight;
}	
int CGif89a::getFrameNum()
{
	return m_gifInfo.frames;
}
int CGif89a::getDelay(int id)
{
	if(id < 0 || id >= m_gifInfo.frames){
		return 0;
	}
	if(m_allFrames == NULL){
		return 0;
	}
	return m_allFrames[id].ctrlExt.delayTime;
}
