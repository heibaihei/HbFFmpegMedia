//
//  CSSpriteService.h
//  Sample
//
//  Created by zj-db0519 on 2018/1/25.
//  Copyright © 2018年 meitu. All rights reserved.
//

#ifndef CSSpriteService_h
#define CSSpriteService_h

#include <stdio.h>
#include <list>
#include "CSMathBase.h"
#include "CSFrameBuffer.h"
#include <OpenGL/gltypes.h>
#include "CSMatrixShader.h"

namespace HBMedia {

typedef class CSMatrixShader CSMatrixShader;
typedef class CSSprite CSSprite;
typedef class CSShaderGroup CSShaderGroup;
typedef class CSTextureShader CSTextureShader;
    
typedef class CSSpriteService {
public:
    CSSpriteService();
    ~CSSpriteService();
    
    /**
     *  屏幕相关初始化
     */
    int prepare();
    /**
     *   设置目标绘制区域大小
     */
    void setScreenSize(int width, int height);
    
    /**
     *  对 mSpritesArray 进行增删改
     */
    void pushSprite(CSSprite* pSprite);
    void popSprite(CSSprite* pSprite);
    
    /**
     *  执行纹理的更新相关操作
     */
    int begin();
    int update();
    int end();
    
    
    
public: /** 配置相关 对外接口 */
    /**
     * enables/disables OpenGL alpha blending
     */
    void setAlphaBlending(bool enable);
    
    /**
     * enables/disables OpenGL depth test
     */
    void setDepthTest(bool enable);
    
protected:
    
    /**
     *  是否需要绘制背景
     *  true 需要 | false 不需要
     */
    bool _CheckNeedDrawBgp(CSSprite* pSprite);
    
    /**
     *  检查是否需要绘制 FBO
     */
    bool _CheckNeedDrawToFBO(CSSprite* pSprite);

protected:
    int  _drawSprite(CSSprite *pSprite);
    int  _updateSprites();
    
    void _bindTextureCache();
    int  _innerGlPrepare();
    void _setMVPMatrix (const Mat4& mvp);
    
    void setupVBO();
    void releaseVBO();
    
   /*
    *  draw shader, dark corner and soft focus
    *
    *  @param sprite  sprite current sprite
    *  @param textureName current textureID, output textureID
    */
    void _drawSpriteShaderToFBO1(const CSSprite* sprite, int& textureName);
    
    /**
     *  @func _screenSizeChanged 屏幕发生改变时，重新计算内部相关参数
     *  @param width  新的屏幕宽
     *  @param height 新的屏幕高
     */
    void _screenSizeChanged(int width, int height);
    
private:
    /**
     *  背景颜色
     */
    Vec3 mFramebufferBackgroundColor;
    
    /**
     *  For TextureCache
     *  Must less than TextureCache::MAX_CACHE
    */
    static int mTextureCachNum;
    int mTextureCacheIndex;

    static const int mTextureVboSize = 8192;
    static const int mTextureVboIndex = (mTextureVboSize * 6 / 4);
    
    NS_GLX::V3F_C4F_T2F mQuadVerts[mTextureVboSize];
    GLushort mQuadIndices[mTextureVboIndex];
    
    GLuint mQuadbuffersVBO[2]; //0: vertex  1: indices
    int    mNumberOfQuads;
    
    CSTextureShader *mCommonShader;
    CSMatrixShader  *mMatrixShader;
    
    /** 存放 sprite 的集合 */
    std::list<CSSprite *> mSpritesArray;
    
    /**
     *  渲染显示区域大小
     */
    int mScreenWidth;
    int mScreenHeight;
    
    /**
     *  绘制区域的大小
     */
    int mRenderWidth;
    int mRenderHeight;
    
    /**
     * FrameBuffer for cache object
     *
     */
    /**
     *  FBO for OpenGL used before.
     *  启动当前Renderer之前生效的 FBO id 保存在此属性上。方便在结束绘制的时候还原现场
     */
    GLint mOldFramebufferObject;
    CSFrameBuffer mFramebufferObject;
    CSFrameBuffer mBitmapFBO;
    CSFlipVerticalShader mFilpShader;
    bool          mEnableFilpShader;
    
    CSFrameBuffer mFragmentFBO;
    CSFrameBuffer mFramebufferObject1;
    CSFrameBuffer mFramebufferObject2;
    
    /** VIDEO_TRACK and PICTURE_TRACK shader */
    CSShaderGroup* mFragmentShader;
    
    Mat4 vMat;
    Mat4 pMat;
    Mat4 mvpMat;
    
    /**
     *  相关转换矩阵
     */
    Mat4 mMVPMatrix;    // Projection * View * Model;
    Mat4 mProjMatrix;   // Proj
    Mat4 mMMatrix;      // Model
    Mat4 mVMatrix;      // View
    
    // 映射到屏幕上View的矩阵，用于计算指定顶点在屏幕内的坐标
    Mat4 targetScreen = Mat4::IDENTITY;
    
    bool mBDrawing;
} CSSpriteService;
    
}
#endif /* CSSpriteService_h */
