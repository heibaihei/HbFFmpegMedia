//
//  CSMediaElement.h
//  Sample
//
//  Created by zj-db0519 on 2018/2/12.
//  Copyright © 2018年 meitu. All rights reserved.
//

#ifndef CSMediaElement_h
#define CSMediaElement_h

#include <stdio.h>
#include "CSDefine.h"
#include "CSMathBase.h"

namespace HBMedia {

typedef class CSTexture CSTexture;
typedef class CSMediaBase CSMediaBase;
typedef class CSShader CSShader;
typedef class CSSprite CSSprite;
typedef class CSSpriteService CSSpriteService;

/**
 *  多媒体播放节点信息
 */
typedef struct EltMediaInfo {
    int64_t mStartPos;
    int64_t mDuration;
    int64_t mLength;
    int64_t mFileStartPos;
    float mSpeed;
    
    float mCenterX;
    float mCenterY;
    int mWidth;
    int mHeight;
    
    /** 逆时针旋转角度: Counterclockwise */
    float mRotateAngle;
    /** Counterclockwise 0 90 180 270 */
    int mContentRotateAngle;
} EltMediaInfo;

void eltMediaInfoInitial(EltMediaInfo *pInfo);

typedef class CSMediaElement {
public:
    CSMediaElement();
    ~CSMediaElement();
    
    int prepare();
    
    int start();
    
    int stop();
    
    int release();
    
    int fetchNextFrame(int64_t clock, CSSpriteService *pSpriteService, STREAM_TYPE eMediaType);
    
public:
    /**
     *  获取媒体信息
     */
    int64_t getStartPos() { return mWorkMediaInfo.mStartPos; }
    int64_t getDuration() { return (mWorkMediaInfo.mDuration + mWorkMediaInfo.mLength); }
    int64_t getSpeed() { return mWorkMediaInfo.mSpeed; }
    
    /** 设置初始宽高 [内部接口] */
    void setOriginalWidthAndHeight(int width, int height);
    
protected:
    
    /**
     *  @func _glPrepare GL资源初始化
     *  @param pSpriteService 渲染服务对象
     */
    int _glPrepare(CSSpriteService *pSpriteService);
    
    /**
     *  设置 shader
     */
    void setShaderOnGL();
    
private:
    /**
     *  多媒体片段信息
     */
    EltMediaInfo mOriginalMediaInfo;
    EltMediaInfo mWorkMediaInfo;
    
    /** UV coordinate */
    float startU, endU; //x
    float startV, endV; //y
    
    /** 裁剪 Scissor */
    Vec2 mScissorLowerLeft;
    NS_GLX::Size mScissorBox;
    bool mEnableScissor;
    
    // for text draw color
    bool mUseColor;
    Vec3 mTextureColor;
    
    float mScaleX;
    float mScaleY;
    
    /**
     *  值越大的，排在越前面，越上层
     */
    int mZOrder;
    
    CSShader* mShader;
    
    FragmentType mTrackType;
    /**
     *  渲染精灵对象
     */
    CSSprite *mSprite;
    CSTexture       *mTexture;
    /**
     *  多媒体片段生产者
     */
    CSMediaBase* mMediaProvider;
} CSMediaElement;
    
}

#endif /* CSMediaElement_h */
