//
//  CSTextureShader.hpp
//  Sample
//
//  Created by zj-db0519 on 2018/1/26.
//  Copyright © 2018年 meitu. All rights reserved.
//

#ifndef CSTextureShader_h
#define CSTextureShader_h

#include <stdio.h>
#include <string>
#include <OpenGL/gltypes.h>
#include "CSMathBase.h"

namespace HBMedia {

typedef class CSTextureShader
{
protected:
    GLint            m_programId;
    std::string        m_vertexShaderCode;
    std::string        m_fragmentShaderCode;
    
    GLint        m_positionAttributeHandle;
    GLint        m_texCoordAttributeHandle;
    GLint        m_samplerHandle;
    GLint        m_matrixHandle;
    GLint        m_useColorHandle;
    GLint        m_textColorHandle;
    GLint        m_alphaHandle;
    
    Mat4 matrix;
    
    int useColor;
    Vec3 textColor;
    
    const int USE_COLOR;
    const int DONOT_USE_COLOR;
    const int JUDGE_USE_COLOR;
    
public:
    CSTextureShader();
    ~CSTextureShader();
    
    bool link();
    
    // set before setup
    void setMatrix(const Mat4& m);
    
    // set before setup
    void setUseColor(bool use);
    
    // set before setup
    void setTextColor(const Vec3& color);
    
    void setup(int texName, float alpha = 1) const;
    
    void release();
} CSTextureShader;

}
#endif /* CSTextureShader_h */
