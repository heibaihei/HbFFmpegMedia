#include "ADD_WaterMark.h"
#include "scale_yuv.h"
#include "rotate_yuv.h"
#include "convert_yuv.h"
#include "MeiPaiGaussBlur.h"
#include "LogHelper.h"
#include <cstdio>

#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) if(p) {delete [] p; p = NULL;}
#endif

#define WATERMARK_STAND_SIZE 480

ADD_WaterMark::ADD_WaterMark()
{
    m_WaterMark_WidthScale = 132.0f/WATERMARK_STAND_SIZE;
    m_WaterMark_HeightScale= 44.0f/WATERMARK_STAND_SIZE;
    m_WaterMark_WidthHeightSca = 44.0f/132.0f;
    m_WaterMark_Height = 0;
    m_WaterMark_Width = 0;
    m_RenderWidth = 0;
    m_RenderHeight = 0;
    m_DstScaleWidth = 0;
    m_DstScaleHeight = 0;
    m_GaussAlpha = 0;
    m_WatermarkApha = 100;
    m_pWatermarkY = NULL;
    m_pWatermarkU = NULL;
    m_pWatermarkV = NULL;
    m_pWatermarkA = NULL;
    
    Dst_YScaleMark = NULL;
    Dst_UScaleMark = NULL;
    Dst_VScaleMark = NULL;
    Dst_AScaleMark = NULL;
}

ADD_WaterMark::~ADD_WaterMark()
{
    SAFE_DELETE_ARRAY(m_pWatermarkY);
    SAFE_DELETE_ARRAY(m_pWatermarkU);
    SAFE_DELETE_ARRAY(m_pWatermarkV);
    SAFE_DELETE_ARRAY(m_pWatermarkA);
    SAFE_DELETE_ARRAY(Dst_YScaleMark);
    SAFE_DELETE_ARRAY(Dst_UScaleMark);
    SAFE_DELETE_ARRAY(Dst_VScaleMark);
    SAFE_DELETE_ARRAY(Dst_AScaleMark);
}

bool ADD_WaterMark::Add_WaterMark_YUV420( MeipaiWatermarkType type,
                                         unsigned char *src_y,int stride_srcy,
                                         unsigned char *src_u,int stride_srcu,
                                         unsigned char *src_v,int stride_srcv,
                                         int src_width,int src_height)
{
    //无数据流
    if(src_y == NULL || src_u == NULL || src_v == NULL || type == WATERMARK_NONE)
    {
        return false;
    }
    //原图宽高不为偶数
    if((src_width&1)||(src_height&1) || src_width == 0 || src_height == 0)
    {
        return false;
    }
    if(m_RenderWidth != src_width || m_RenderHeight != src_height)
    {
        if(!this->SetRenderPictureSize(src_width,src_height))
        {
            LOGE("wfc error: SetRenderPictureSize failed.");
            return false;
        }
    }
    if(m_GaussAlpha)
    {
        //TODO:做高斯
        CreateBlurEffectInt(src_y,src_width,src_height,10.0f,10.0f,m_GaussAlpha);
        CreateBlurEffectInt(src_u,src_width/2,src_height/2,10.0f,10.0f,m_GaussAlpha);
        CreateBlurEffectInt(src_v,src_width/2,src_height/2,10.0f,10.0f,m_GaussAlpha);
    }
    //寻找水印起始点
    int yindex = 0;
    int ymindex = 0;
    int uindex = 0;
    int umindex = 0;
    int vindex = 0;
    int vmindex = 0;
    int st_height=0;
    int st_width=0;
    //融合
    switch(type)
    {
        case WATERMARK_TOP_LEFT:	//左上
            yindex = 0;
            ymindex = 0;
            uindex = 0;
            umindex = 0;
            vindex = 0;
            vmindex = 0;
            break;
        case WATERMARK_TOP_RIGHT:	//右上
            yindex = src_width - m_DstScaleWidth;
            ymindex = 0;
            uindex = (src_width - m_DstScaleWidth)/2;
            umindex = 0;
            vindex = (src_width - m_DstScaleWidth)/2;
            vmindex = 0;
            break;
        case WATERMARK_BOTTOM_LEFT:	//左下
            yindex = (src_height-m_DstScaleHeight)*stride_srcy;
            ymindex = 0;
            uindex = (src_height-m_DstScaleHeight)/2*(stride_srcu);
            umindex = 0;
            vindex = (src_height-m_DstScaleHeight)/2*(stride_srcv);
            vmindex = 0;
            break;
        case WATERMARK_BOTTOM_RIGHT: //右下
            yindex = (src_height-m_DstScaleHeight)*stride_srcy+(src_width-m_DstScaleWidth);
            ymindex = 0;
            uindex = (src_height-m_DstScaleHeight)/2*(stride_srcu)+(src_width-m_DstScaleWidth)/2;
            umindex = 0;
            vindex = (src_height-m_DstScaleHeight)/2*(stride_srcv)+(src_width-m_DstScaleWidth)/2;
            vmindex = 0;
            break;
        case WATERMARK_CENTER:
            st_height=(src_height-m_DstScaleHeight)/2;
            if(st_height&1)st_height--;
            st_width=(src_width-m_DstScaleWidth)/2;
            if(st_width&1)st_width--;
            yindex = st_height*stride_srcy+st_width;
            ymindex = 0;
            uindex = st_height/2*(stride_srcu)+(st_width)/2;
            umindex = 0;
            vindex = st_height/2*(stride_srcv)+(st_width)/2;
            vmindex = 0;
            break;
        default:
            break;
    }
    for(int i = 0 ; i < m_DstScaleHeight ; i++)
    {
        int offsety = yindex;
        int offsetym = ymindex;
        for(int j = 0 ; j < m_DstScaleWidth ; j++)
        {
            int alpha = Dst_AScaleMark[offsetym];
            alpha = alpha*m_WatermarkApha/100;
            if(alpha != 0)
            {
                src_y[offsety] = (src_y[offsety] * (255-alpha) + Dst_YScaleMark[offsetym] * alpha)>>8;
            }
            offsety++;
            offsetym++;
        }
        yindex+=stride_srcy;
        ymindex+=m_DstScaleWidth;
    }
    for(int i = 0 ; i < m_DstScaleHeight ; i += 2)
    {
        int offsetuv = 0;
        for(int j = 0 ; j < m_DstScaleWidth ; j += 2)
        {
            int index = i*m_DstScaleWidth + j;
            
            int alpha = (Dst_AScaleMark[index] + Dst_AScaleMark[index+1] + Dst_AScaleMark[index+m_DstScaleWidth] + Dst_AScaleMark[index + 1 + m_DstScaleWidth])>>2;
            alpha = alpha*m_WatermarkApha/100;
            if(alpha != 0)
            {
                src_u[uindex+offsetuv] = (src_u[uindex+offsetuv]*(255-alpha) + Dst_UScaleMark[umindex+offsetuv]*alpha)>>8;
                src_v[vindex+offsetuv] = (src_v[vindex+offsetuv]*(255-alpha) + Dst_VScaleMark[vmindex+offsetuv]*alpha)>>8;
            }
            offsetuv++;
        }
        uindex += stride_srcu;
        vindex += stride_srcv;
        umindex += m_DstScaleWidth/2;
        vmindex += m_DstScaleWidth/2;
    }
    return true;
}


bool ADD_WaterMark::LoadWatermark( const char* watermark_path )
{
    //文件名不为空
    if(watermark_path == NULL)
    {
        return false;
    }
    
    int nPixelCount = 0;
    FILE *infile = fopen(watermark_path,"rb");
    //打开失败
    if(infile == NULL)
    {
        return false;
    }
    
    //写入图片大小
    fread(&m_WaterMark_Width,sizeof(int),1,infile);
    fread(&m_WaterMark_Height,sizeof(int),1,infile);
    
    if(m_WaterMark_Width %2 || m_WaterMark_Height %2 || m_WaterMark_Width == 0 || m_WaterMark_Height == 0
       || m_WaterMark_Height > WATERMARK_STAND_SIZE || m_WaterMark_Width > WATERMARK_STAND_SIZE)
    {
        goto FAIL;
    }
    
    
    if(m_WaterMark_Width %2 || m_WaterMark_Height %2 || m_WaterMark_Width == 0 || m_WaterMark_Height == 0)
    {
        goto FAIL;
    }
    
    nPixelCount = m_WaterMark_Height*m_WaterMark_Width;
    m_pWatermarkY = new unsigned char[nPixelCount];
    m_pWatermarkU = new unsigned char[nPixelCount/4];
    m_pWatermarkV = new unsigned char[nPixelCount/4];
    m_pWatermarkA = new unsigned char[nPixelCount];
    
    //读入YUV
    fread(m_pWatermarkY,sizeof(unsigned char),nPixelCount,infile);
    fread(m_pWatermarkU,sizeof(unsigned char),nPixelCount/4,infile);
    fread(m_pWatermarkV,sizeof(unsigned char),nPixelCount/4,infile);
    fread(m_pWatermarkA,sizeof(unsigned char),nPixelCount,infile);
    
    //关闭文件流
    fclose(infile);
    
    m_WaterMark_WidthScale = (float)m_WaterMark_Width / WATERMARK_STAND_SIZE;
    m_WaterMark_HeightScale = (float)m_WaterMark_Height / WATERMARK_STAND_SIZE;
    m_WaterMark_WidthHeightSca = (float)m_WaterMark_Height/m_WaterMark_Width;
    m_RenderWidth = 0;
    m_RenderHeight = 0;
    return true;
FAIL:
    if (infile != NULL) {
        fclose(infile);
    }
    
    return false;
}

bool ADD_WaterMark::SetRenderPictureSize( int width,int height )
{
    if((width&1) || (height&1))
    {
        return false;
    }
    
    m_RenderWidth = width;
    m_RenderHeight = height;
    
    //------固定的比例
    m_DstScaleWidth = m_RenderWidth * m_WaterMark_WidthScale;
    m_DstScaleHeight = m_DstScaleWidth*m_WaterMark_WidthHeightSca;
    //special judge
    if(m_DstScaleHeight>m_RenderHeight+1){
        m_DstScaleHeight=m_RenderHeight * m_WaterMark_HeightScale;
        m_DstScaleWidth =m_DstScaleHeight/m_WaterMark_WidthHeightSca;
    }
    //排除偶数宽高;
    if(m_DstScaleWidth&1)
    {
        m_DstScaleWidth--;
    }
    if(m_DstScaleHeight&1)
    {
        m_DstScaleHeight--;
    }
    //height width flow
    if(m_DstScaleHeight > m_RenderHeight || m_DstScaleWidth > m_RenderWidth || m_DstScaleWidth == 0 ||
       m_DstScaleHeight == 0)
    {
        LOGE("wfc m_DstScaleHeight = %d, m_RenderHeight = %d, m_DstScaleWidth = %d, m_RenderWidth = %d", m_DstScaleHeight, m_RenderHeight, m_DstScaleWidth, m_RenderWidth);
        return false;
    }
    SAFE_DELETE_ARRAY(Dst_YScaleMark);
    SAFE_DELETE_ARRAY(Dst_UScaleMark);
    SAFE_DELETE_ARRAY(Dst_VScaleMark);
    SAFE_DELETE_ARRAY(Dst_AScaleMark);
    int nScalePixelCount = m_DstScaleWidth*m_DstScaleHeight;
    Dst_YScaleMark=new unsigned char[nScalePixelCount];
    Dst_UScaleMark=new unsigned char[nScalePixelCount/2];
    Dst_VScaleMark=new unsigned char[nScalePixelCount/2];
    Dst_AScaleMark=new unsigned char[nScalePixelCount];
    
    //放缩ALPHA
    ScalePlane(
               m_pWatermarkA,m_WaterMark_Width,
               m_WaterMark_Width,m_WaterMark_Height,
               Dst_AScaleMark,m_DstScaleWidth,
               m_DstScaleWidth,m_DstScaleHeight,
               kFilterBilinear
               );
    
    //放缩水印
    I420Scale(
              m_pWatermarkY,m_WaterMark_Width,
              m_pWatermarkU,m_WaterMark_Width/2,
              m_pWatermarkV,m_WaterMark_Width/2,
              m_WaterMark_Width,m_WaterMark_Height,
              Dst_YScaleMark,m_DstScaleWidth,
              Dst_UScaleMark,m_DstScaleWidth/2,
              Dst_VScaleMark,m_DstScaleWidth/2,
              m_DstScaleWidth,m_DstScaleHeight,
              kFilterBilinear
              );
    return true;
}

//仅支持四通道
bool ADD_WaterMark::EncoderYUV420(
                                  const char* save_path,
                                  unsigned char *src_y,
                                  unsigned char *src_u,
                                  unsigned char *src_v,
                                  unsigned char *src_a,
                                  int src_width,int src_height )
{
    if(src_y == NULL || src_u == NULL || src_v == NULL || src_a == NULL)
        return false;
    
    if((src_width%2) || (src_height%2) )
        return false;
    
    //文件名不为空
    if(save_path == NULL)
        return false;
    
    FILE *outfile = fopen(save_path,"wb");
    
    //打开失败
    if(outfile == NULL)
        return false;
    //写入图片大小
    fwrite(&src_width,sizeof(int),1,outfile);
    fwrite(&src_height,sizeof(int),1,outfile);
    //写入YUV
    fwrite(src_y,sizeof(unsigned char),src_width*src_height,outfile);
    fwrite(src_u,sizeof(unsigned char),src_width/2*src_height/2,outfile);
    fwrite(src_v,sizeof(unsigned char),src_width/2*src_height/2,outfile); 
    fwrite(src_a,sizeof(unsigned char),src_width*src_height,outfile);
    
    //关闭文件流
    fclose(outfile);
    return true;
}

bool ADD_WaterMark::EncodeWatermarkToFile( const char* save_path,unsigned char* pRgba,int stride,int src_width,int src_height )
{
    if((src_width&1) || (src_height&1))
    {
        LOGE("error: meipai water mark image width or height must be even number.");
        return false;
    }
    
    unsigned char* DST_Y = new unsigned char[src_width*src_height];
    unsigned char* DST_A = new unsigned char[src_width*src_height];
    unsigned char* DST_U = new unsigned char[src_width*src_height/4];
    unsigned char* DST_V = new unsigned char[src_width*src_height/4];
    unsigned char* pTmpData = pRgba;
    for(int i = 0 ; i < src_width*src_height ; i++)
    {
        DST_A[i] = pTmpData[MT_ALPHA];
        pTmpData += 4;
    }
    //ARGBToI420
    ARGBToI420(pRgba,stride,DST_Y,src_width,DST_U,src_width/2,DST_V,src_width/2,src_width,src_height);
    
    if(!ADD_WaterMark::EncoderYUV420(save_path,DST_Y,DST_U,DST_V,DST_A,src_width,src_height))
    {
        SAFE_DELETE_ARRAY(DST_Y);
        SAFE_DELETE_ARRAY(DST_U);
        SAFE_DELETE_ARRAY(DST_V);
        SAFE_DELETE_ARRAY(DST_A);
        
        return false;
    }
    SAFE_DELETE_ARRAY(DST_Y);
    SAFE_DELETE_ARRAY(DST_U);
    SAFE_DELETE_ARRAY(DST_V);
    SAFE_DELETE_ARRAY(DST_A);
    return true;
}

bool ADD_WaterMark::SetWatermrkParam( int gauss,int alpha )
{
    m_GaussAlpha = gauss;
    m_WatermarkApha = alpha;
    return true;
}
