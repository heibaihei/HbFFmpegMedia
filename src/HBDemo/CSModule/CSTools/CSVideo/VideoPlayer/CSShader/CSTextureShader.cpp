//
//  CSTextureShader.cpp
//  Sample
//
//  Created by zj-db0519 on 2018/1/26.
//  Copyright © 2018年 meitu. All rights reserved.
//

#include "CSTextureShader.h"

#include "CSGlUtil.h"
#include "CSTexture.h"

namespace HBMedia {

CSTextureShader::CSTextureShader()
: USE_COLOR(10), DONOT_USE_COLOR(0), JUDGE_USE_COLOR(5)
{
    m_programId = CSGlUtils::INVALID;
    
    m_positionAttributeHandle = -1;
    m_texCoordAttributeHandle = -1;
    m_samplerHandle = -1;
    m_matrixHandle = -1;
    m_useColorHandle = -1;
    m_textColorHandle = -1;
    
    m_useColor = DONOT_USE_COLOR;
    
    m_vertexShaderCode =
    "attribute vec4 a_vPosition;        \n"
    "attribute vec2 a_texCoord;         \n"
    "varying   vec2 v_texCoord;         \n"
    "uniform   mat4 u_Matrix;           \n"
    "void main(){                       \n"
    "    gl_Position = u_Matrix * a_vPosition;        \n"
    "    v_texCoord = a_texCoord;       \n"
    "}                                  \n";
    
    m_fragmentShaderCode =
    "#ifdef GL_ES\n"
    "precision mediump float;\n"    // specify float precision.
    "#else\n"
    "#define highp\n"
    "#endif\n"
    "varying highp vec2 v_texCoord;                          \n"
    "uniform sampler2D s_texture;                            \n"
    "uniform float u_useColor;                               \n"
    "uniform vec3 u_textColor;                               \n"
    "uniform float u_alpha;                                  \n"
    "void main(){                                            \n"
    "    vec4 src = texture2D(s_texture, v_texCoord);        \n"
    "    if (u_useColor < 5.0) {                             \n"
    "        gl_FragColor = vec4(src.rgb, src.a*u_alpha);    \n"
    "    } else {                                            \n"
    "        gl_FragColor = vec4(u_textColor, src.a*u_alpha);\n"
    "    }                                                   \n"
    "}                                                       \n";
}

CSTextureShader::~CSTextureShader() {
}

bool CSTextureShader::link()
{
    if (m_programId != CSGlUtils::INVALID)
        return true;
    
    int m_vertexShaderId = CSGlUtils::loadShader(GL_VERTEX_SHADER, m_vertexShaderCode);
    int m_fragmentShaderId = CSGlUtils::loadShader(GL_FRAGMENT_SHADER, m_fragmentShaderCode);
    m_programId = CSGlUtils::createProgram(m_vertexShaderId, m_fragmentShaderId);
    glDeleteShader(m_vertexShaderId);
    glDeleteShader(m_fragmentShaderId);
    
    if (m_programId == CSGlUtils::INVALID) {
        LOGE("Texture shader >>> invalid program id !");
        return false;
    }
    
    m_positionAttributeHandle   = glGetAttribLocation(m_programId, "a_vPosition");
    m_texCoordAttributeHandle   = glGetAttribLocation(m_programId, "a_texCoord");
    m_samplerHandle             = glGetUniformLocation(m_programId, "s_texture");
    m_matrixHandle              = glGetUniformLocation(m_programId, "u_Matrix");
    m_useColorHandle            = glGetUniformLocation(m_programId, "u_useColor");
    m_textColorHandle           = glGetUniformLocation(m_programId, "u_textColor");
    m_alphaHandle               = glGetUniformLocation(m_programId, "u_alpha");
    return true;
}

void CSTextureShader::setMatrix(const Mat4& m) {
    m_matrix = m;
}

void CSTextureShader::setUseColor(bool use) {
    m_useColor = use ? USE_COLOR : DONOT_USE_COLOR;
}

void CSTextureShader::setTextColor(const Vec3& color) {
    m_textColor = color;
}

void CSTextureShader::setup(int texName, float alpha) const {
    glUseProgram(m_programId);
    
    glEnableVertexAttribArray(m_positionAttributeHandle);
    glEnableVertexAttribArray(m_texCoordAttributeHandle);
    // vertices
    glVertexAttribPointer(m_positionAttributeHandle, 3, GL_FLOAT, GL_FALSE, V3F_C4F_T2F_SIZE, (GLvoid*) Vertex3F_OFFSET);
    // tex coords
    glVertexAttribPointer(m_texCoordAttributeHandle, 2, GL_FLOAT, GL_FALSE, V3F_C4F_T2F_SIZE, (GLvoid*) Tex2F_OFFSET);
    
    glUniformMatrix4fv(m_matrixHandle, 1, GL_FALSE, m_matrix.m);
    
    glUniform1f(m_useColorHandle, m_useColor);
    
    glUniform3f(m_textColorHandle, m_textColor.x, m_textColor.y, m_textColor.z);
    
    glUniform1f(m_alphaHandle, alpha);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texName);
    glUniform1i(m_samplerHandle, 0);
}

void CSTextureShader::release() {
    if (m_programId != CSGlUtils::INVALID) {
        glDeleteProgram(m_programId);
        m_programId = CSGlUtils::INVALID;
    }
}

}
