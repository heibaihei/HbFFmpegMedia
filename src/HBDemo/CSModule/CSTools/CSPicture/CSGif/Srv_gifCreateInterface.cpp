#include "Srv_gifCreateInterface.h"
#include "gif/MTGIF.h"

CSGifEncoder::CSGifEncoder()
{
	m_cmtGif = NULL;
}

CSGifEncoder::~CSGifEncoder()
{
	if(m_cmtGif != NULL)
    {
        delete m_cmtGif ;
        m_cmtGif = NULL;
    }
}

bool CSGifEncoder::WriteGifHeader(char* gifFilePath, int frameCount, int width, int height, int time)
{
    m_cmtGif  = new CMTGIF();
    return m_cmtGif->WriteGifHeader(gifFilePath, frameCount, width, height, time);
}

bool CSGifEncoder::SendGifFrame(BYTE *frameData)
{
    return m_cmtGif->SendGifFrame(frameData);
}
    
bool CSGifEncoder::Close()
{
    return m_cmtGif->Close();
}


