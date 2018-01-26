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

namespace HBMedia {

typedef class CSSprite CSSprite;
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
    void _bindTextureCache();
    int  _innerGlPrepare();
    void _setMVPMatrix (const Mat4& mvp);
    
    void setupVBO();
    void releaseVBO();
    void mapBuffers();
    void _screenSizeChanged(int width, int height);
    
private:
    /**
     *  For TextureCache
     *  Must less than TextureCache::MAX_CACHE
    */
    static int mTextureCachNum;
    int mTextureCacheIndex;

    static const int mTextureVboSize = 8192;
    static const int mTextureVboIndex = mTextureVboSize * 6 / 4;
    
    NS_GLX::V3F_C4F_T2F mQuadVerts[mTextureVboSize];
    GLushort mQuadIndices[mTextureVboSize];
    
    GLuint mQuadbuffersVBO[2]; //0: vertex  1: indices
    
    CSTextureShader* mCommonShader;
    
    /** 存放 sprite 的集合 */
    std::list<CSSprite *> mSpritesArray;
    
    /**
     *  渲染区域大小
     */
    int mScreenWidth;
    int mScreenHeight;
    
    /**
     *  绘制区域的大小
     */
    int mRenderWidth;
    int mRenderHeight;
    
    CSFrameBuffer mFragmentFBO;
    CSFrameBuffer mFramebufferObject1;
    CSFrameBuffer mFramebufferObject2;
    
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
} CSSpriteService;
    
}
#endif /* CSSpriteService_h */
