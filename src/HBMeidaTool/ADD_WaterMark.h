#pragma  once

typedef enum MeipaiWatermarkType{
    WATERMARK_NONE,
    WATERMARK_TOP_LEFT,		//左上
    WATERMARK_TOP_RIGHT,	//右上
    WATERMARK_BOTTOM_LEFT,//左下
    WATERMARK_BOTTOM_RIGHT,//右下
    WATERMARK_CENTER,	//居中
}MeipaiWatermarkType;

class ADD_WaterMark{
private:
    
    
    
    float m_WaterMark_WidthScale;
    float m_WaterMark_HeightScale;
    float m_WaterMark_WidthHeightSca;
    /////////////////////////水印///////////////////////////////////////////////
    int m_WaterMark_Width;
    int m_WaterMark_Height;
    unsigned char* m_pWatermarkY;
    unsigned char* m_pWatermarkU;
    unsigned char* m_pWatermarkV;
    unsigned char* m_pWatermarkA;
    //缩放完后的水印
    int m_DstScaleWidth;
    int m_DstScaleHeight;
    unsigned char*  Dst_YScaleMark;
    unsigned char*  Dst_UScaleMark;
    unsigned char*  Dst_VScaleMark;
    unsigned char*  Dst_AScaleMark;
    int m_RenderWidth,m_RenderHeight;
    //水印的透明度
    int m_GaussAlpha;
    int m_WatermarkApha;
    //////////////////////////////////////////////////////////////////////////
private:
    //设置要渲染的图片宽高;
    //
    bool SetRenderPictureSize(int width,int height);
    
    static bool EncoderYUV420(const char* save_path,
                              unsigned char *src_y,
                              unsigned char *src_u,
                              unsigned char *src_v,
                              unsigned char *src_a,
                              int src_width,int src_height);
public:
    ADD_WaterMark();
    ~ADD_WaterMark();
    //加载水印文件
    //@param watermark_path :文件路径
    bool LoadWatermark(const char* watermark_path);
    //设置水印的参数
    //@param gauss 高斯的程度 [0-100]
    //@param alpha 水印的透明度[0-100]
    bool SetWatermrkParam(int gauss,int alpha);
    //---------------------YUV420混合水印函数----------------------
    //方法（具体看枚举型）
    //src_y,u,v ：原图的YUV
    //stride_y,u,v 原图YUV步长
    //src_width,src_height 原图大小
    //函数功能：将水印放缩对应比例，按照水印放置方式放到图片位置
    //输出：原图的YUV通道
    //说明：由于转换格式为YUV420,原图的宽度和高度必须均为偶数,水印的宽度和高度均为偶数
    bool Add_WaterMark_YUV420(MeipaiWatermarkType type,
                              unsigned char *src_y,int stride_srcy,
                              unsigned char *src_u,int stride_srcu,
                              unsigned char *src_v,int stride_srcv,
                              int src_width,int src_height
                              );
    //---------------------------------------------------------------------
    //Encode 文件接口
    static bool EncodeWatermarkToFile( const char* save_path,unsigned char* pRgba,int stride,int src_width,int src_height );
    
};



