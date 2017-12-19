#ifndef __SRV_GIFCREATERFACE__H__
#define __SRV_GIFCREATERFACE__H__

class CMTGIF;

typedef class CSGifEncoder
{
public :
    CSGifEncoder();
    ~CSGifEncoder();
    
    /**
     *  写入 gif 头
     *  @param gifFilePath 目标输出文件名
     *  @param ncount      预期写入的图像帧的个数
     *  @param
     */
    bool WriteGifHeader(char* gifFilePath, int frameCount, int width, int height, int time);

    bool SendGifFrame(unsigned char *frameData);
	
	bool Close();
   	
private:
	CMTGIF * m_cmtGif;
} CSGifEncoder;

#if 0

/** 代码提交 相关调用demo模式 */
CSGifEncoder* mGifCreate = new CSGifEncoder();
mGifCreate->WriteGifHeader(RESOURCE_ROOT_PATH"/video/newGifOutput.gif", 299, \
                           pVideoConverter->getOutputImageParams()->mWidth, \
                           pVideoConverter->getOutputImageParams()->mHeight, 3);


int HbErr = 0;
uint8_t *pTargetImageData = nullptr;
int64_t  timeStamp = 0;
int iCount = 0;
while (true) {
    HbErr = pVideoConverter->receiveFrame(&pTargetImageData, &timeStamp);
    if (HbErr == -2) { /** 解码完成退出 */
        break;
    }
    else if (HbErr == 0) { /** 得到帧数据 */
        printf("[Huangcl] ---> %d\r\n", ++iCount);
        mGifCreate->SendGifFrame(pTargetImageData);
        free(pTargetImageData);
    }
}

/** 解码结束后，必须调用stop 回收线程资源 */
pVideoConverter->stop();
mGifCreate->Close();

#endif

#endif 
