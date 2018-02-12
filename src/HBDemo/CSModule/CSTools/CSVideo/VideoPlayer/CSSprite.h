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

typedef class CSSpriteService CSSpriteService;
typedef class CSShader CSShader;
typedef class CSTexture CSTexture;
    
typedef class CSSprite {
    friend class CSSpriteService;
public:
    CSSprite();
    
    virtual ~CSSprite();
    
    /**
     *  更新 sprite 中的纹理
     */
    int update();
    
public:
    /** 设置优先级 */
    void setZOrder(int order) { mZOrder = order; }
    int getZOrder() { return mZOrder; }
    
    /**
     *  Shader 相关
     */
    bool hasShader() const;
    void setShader(CSShader* pShader);
    CSShader* getShader() const;
    
    void setTrackType(FragmentType type);
    const FragmentType& getTrackType() const { return mTrackType; };
    /**
     *  获取当前精灵的宽高
     */
    float getWidth() const { return mWidth; };
    float getHeight() const { return mHeight; };
    void setWidthAndHeight(float width, float height);
    void setOrigWidthAndHeight(float width, float height);
    
    /** 0 -90 -180 -270 */
    void setOrigRotateAngle(int angle);
    int getOrigRotateAngle() const { return origRotateAngle; }
    
    int getRotateAngle() const { return rotateAngle; }
    void rotateTo(float angle);
    void rotateBy(float angle);
    
    
    /**
     *  设置当前精灵是否可见
     */
    bool isVisible() const { return mVisible; };
    void setVisible(bool visible) { mVisible = visible; };
    
    const NS_GLX::V3F_C4F_T2F_Quad* getQuads() const { return &mQuads; };
    
    /** 设置外部纹理 */
    void setTexture(CSTexture* texture);
    CSTexture* getTexture() const { return mTexture; };
    
    void moveTo(float cX, float cY);
    void moveBy(float dX, float dY);
    
    /**
     * Scale x,y direction with the same factor to scale.
     *
     * @param scale The factor to scale
     */
    void scaleTo(float scale);
    /**
     * Scale sprite with different value.
     *
     * @param sx The factor to scale by in the x direction.
     * @param sy The factor to scale by in the y direction.
     */
    void scaleTo(float sx, float sy);
    void scaleBy(float scale);
    
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
    void setUV(float sU, float eU, float sV, float eV);
    
    /**
     *  the lower left corner of the scissor box
     *  releative center of sprite
     *
     */
    void setScissorBox(const Vec2& lowerLeft, const NS_GLX::Size& box, bool enableScissor = true);
    const Vec2& getScissorLowerLeft() const { return mScissorLowerLeft; }
    const NS_GLX::Size& getScissorBox() const { return mScissorBox; }
    void setAnimationScissorBox(const Vec2& lowerLeft, const NS_GLX::Size& box, bool enableScissor = true);
    const Vec2& getFinalScissorLowerLeft() const { return mFinalScissorLowerLeft; }
    const NS_GLX::Size& getFinalScissorBox() const { return mFinalScissorBox; }
    bool needEnableScissor() const { return mFinalEnableScissor; }
    
protected:
    
    /**
     *  更新纹理坐标
     */
    int _updatePosition();
    
    /**
     *  更新 UV 坐标
     */
    int _updateUV();
    
    /**
     *  释放资源
     */
    void _release();
private:
    /**
     *  用于表示当前GraphicsSprite 是否要显示： true标识显示｜false表示隐藏
     */
    bool mVisible;

    /**
     *  值越大的，排在越前面，越上层
     */
    int mZOrder;
    
    /** UV coordinate */
    float startU, endU; //x
    float startV, endV; //y
    
    float aStartUChange, aEndUChange;
    float aStartVChange, aEndVChange;
    
    /** sprite 当前中心点以及宽高信息 */
    float mCenterX, mCenterY;
    float mWidth, mHeight;
    /** sprite original w h */
    float mOrigWidth, mOrigHeight;
    
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
    FragmentType mTrackType;
    
    /** for text draw color */
    bool mUseColor;
    Vec3 mTextureColor;
    /** alpha for animation */
    float aAlpha;
    bool mAlphaPremultiplied;
    
    /** releative the center of sprite */
    Vec2 mScissorLowerLeft;
    NS_GLX::Size mScissorBox;
    bool mEnableScissor;
    /** after do ScissorAnimation */
    Vec2 mFinalScissorLowerLeft;
    NS_GLX::Size mFinalScissorBox;
    bool mFinalEnableScissor;
    
    CSTexture *mTexture;
    
    /** sprite shader */
    CSShader *mShader;
    /** for ShaderAnimation */
    std::list<CSShader *> mAnimationShaders;
} CSSprite;
    
}

#endif /* CSSprite_h */
