//
//  CSSprite.h
//  Sample
//
//  Created by zj-db0519 on 2018/1/25.
//  Copyright © 2018年 meitu. All rights reserved.
//

#ifndef CSSprite_h
#define CSSprite_h

#include <stdio.h>
#include <list>
#include "CSMathBase.h"

namespace HBMedia {

typedef class CSShader CSShader;
typedef class CSTexture CSTexture;

/***
 *   定义枚举类型
 */
typedef enum SpriteType {
    UNKNOW_TRACK,
    NORMAL_TRACK,        // 图片或者视频 没有背景
    VIDEO_TRACK,         // 视频
    MUSIC_TRACK,         // 音乐
    PICTURE_TRACK,       // 图片
    SUBTITLE_TRACK,      // 字幕
    TEXT_TEMPLATE_TRACK, // 文字模版
    WATERMARK_TRACK,     // 水印
    VIDEO_MATERIAL_TRACK,// 视频素材
} SpriteType;
    
typedef class CSSprite {
public:
    CSSprite();
    
    virtual ~CSSprite();
    
    /**
     *  更新 sprite 中的纹理
     */
    int update();
    
public:
    
    /**
     *  Shader 相关
     */
    bool hasShader() const;
    void setShader(CSShader* pShader);
    CSShader* getShader() const;
    
    void setTrackType(SpriteType type);
    const SpriteType& getTrackType() const { return mTrackType; };
    /**
     *  获取当前精灵的宽高
     */
    float getWidth() const { return mWidth; };
    float getHeight() const { return mHeight; };
    
    // 0 -90 -180 -270
    void setOrigRotateAngle(int angle);
    int getOrigRotateAngle() const { return origRotateAngle; }
    
    /**
     *  设置当前精灵是否可见
     */
    bool isVisible() const { return mVisible; };
    void setVisible(bool visible) { mVisible = visible; };
    
    const NS_GLX::V3F_C4F_T2F_Quad* getQuads() const { return &mQuads; };
    
    /** 设置外部纹理 */
    void setTexture(CSTexture* texture);
    CSTexture* getTexture() const { return mTexture; };
    
    void setUseColor(bool use);
    bool needUseColor() const { return mUseColor; };
    
    void setTextColor(const Vec3& color);
    const Vec3& getTextColor() const { return mTextureColor; };
    
    void setAnimationAlpha(float alpha);
    float getAnimationAlpha() const { return aAlpha; };
    
    /** for png image */
    void setAlphaPremultiplied(bool premultiplied);
    bool isAlphaPremultiplied() const { return mAlphaPremultiplied; };
    
    float getStartU() const { return startU; };
    float getEndU() const { return endU; };
    float getStartV() const { return startV; };
    float getEndV() const { return endV; };
    
protected:
    
    /**
     *  更新纹理坐标
     */
    int _updatePosition();
    
    /**
     *  更新 UV 坐标
     */
    int _updateUV();
    
private:
    /**
     *  用于表示当前GraphicsSprite 是否要显示： true标识显示｜false表示隐藏
     */
    bool mVisible;

    /** UV coordinate */
    float startU, endU; //x
    float startV, endV; //y
    
    float aStartUChange, aEndUChange;
    float aStartVChange, aEndVChange;
    
    /** sprite 当前中心点以及宽高信息 */
    float mCenterX, mCenterY;
    float mWidth, mHeight;
    
    /** animation 相关动画调整 */
    float aXChange, aYChange;
    float aWChange, aHChange;
    float aWScale, aHScale;
    
    /** 旋转角度：(逆时针方向) Counterclockwise */
    float rotateAngle;
    // 0 -90 -180 -270
    int origRotateAngle;
    
    float aRotateChange;
    bool aUseRotateCenter;
    Vec2 aRotateCenter;
    
    /** position color uv */
    NS_GLX::V3F_C4F_T2F_Quad mQuads;
    
    /**
     *  是否需要更新纹理数据
     */
    bool mNeedUpdatePosition;
    bool mNeedUpdateUV;
    
    /** sprite 类型 */;
    SpriteType mTrackType;
    
    /** for text draw color */
    bool mUseColor;
    Vec3 mTextureColor;
    /** alpha for animation */
    float aAlpha;
    bool mAlphaPremultiplied;
    
    CSTexture *mTexture;
    
    /** sprite shader */
    CSShader *mShader;
    /** for ShaderAnimation */
    std::list<CSShader *> mAnimationShaders;
} CSSprite;
    
}

#endif /* CSSprite_h */
