//
//  CSMatrixShader.cpp
//  Sample
//
//  Created by zj-db0519 on 2018/1/26.
//  Copyright © 2018年 meitu. All rights reserved.
//

#include "CSMatrixShader.h"
#include "CSMatrixShader.h"

namespace HBMedia {
    
const std::string CSMatrixShader::MATRIX_ROTATE_X = "RotateX";
const std::string CSMatrixShader::MATRIX_ROTATE_Y = "RotateY";
const std::string CSMatrixShader::MATRIX_ROTATE_Z = "RotateZ";
const std::string CSMatrixShader::MATRIX_MOVE_X = "MoveX";
const std::string CSMatrixShader::MATRIX_MOVE_Y = "MoveY";
const std::string CSMatrixShader::MATRIX_SCALE_X = "ScaleX";
const std::string CSMatrixShader::MATRIX_SCALE_Y = "ScaleY";

const std::string CSMatrixShader::MATRIX_VERTEX_SHADER =
"attribute vec4 aPosition;\n"
"attribute vec4 aTextureCoord;\n"
"varying vec2 vTextureCoord;\n"
"uniform mat4 uMVPMatrix;\n"
"void main() {\n"
"gl_Position = uMVPMatrix * aPosition;\n"
"vTextureCoord = aTextureCoord.xy;\n"
"}\n";

const std::string CSMatrixShader::ALPHAONE_FRAGMENT_SHADER =
"#ifdef GL_ES\n"
"precision mediump float;\n"    // specify float precision.
"#else\n"
"#define highp\n"
"#endif\n"
"varying highp vec2 vTextureCoord;\n"
"uniform sampler2D sTexture;\n"
"void main() {\n"
"gl_FragColor = texture2D(sTexture, vTextureCoord);\n"
"gl_FragColor.a = 1.0;\n"
"}\n";

const std::string CSMatrixShader::UNIFORM_MVPMATRIX = "uMVPMatrix";

CSMatrixShader::CSMatrixShader(bool useAlpha)
:CSOneInputShader(MATRIX_VERTEX_SHADER, getFragmentShader(useAlpha)) {
    mShaderType = MATRIX_SHADER;
}

const std::string& CSMatrixShader::getFragmentShader(bool useAlpha) {
    if (useAlpha) {
        return DEFAULT_FRAGMENT_SHADER;
    } else {
        return ALPHAONE_FRAGMENT_SHADER;
    }
}

void CSMatrixShader::setShaderData(const std::string &name, const void *data)
{
    float value = *((float*)data);
    if(name.compare(MATRIX_ROTATE_X) == 0){
        Mat4::createRotationX(value, &mvpMatrix);
    } else if(name.compare(MATRIX_ROTATE_Y) == 0){
        Mat4::createRotationY(value, &mvpMatrix);
    } else if(name.compare(MATRIX_ROTATE_Z) == 0){
        Mat4::createRotationZ(value, &mvpMatrix);
    } else if(name.compare(MATRIX_MOVE_X) == 0){
        Mat4::createTranslation(value, 0, 0, &mvpMatrix);
    } else if(name.compare(MATRIX_MOVE_Y) == 0){
        Mat4::createTranslation(0, value, 0, &mvpMatrix);
    } else if(name.compare(MATRIX_SCALE_X) == 0) {
        Mat4::createScale(value, 1.0f, 1.0f, &mvpMatrix);
    } else if(name.compare(MATRIX_SCALE_Y) == 0) {
        Mat4::createScale(1.0f, value, 1.0f, &mvpMatrix);
    }
    
    Mat4::multiply(pMat, mvpMatrix, &mvpMatrix);
}

void CSMatrixShader::setProjection(float width, float height) {
    float right = width / 2;
    float left = -right;
    float top = height / 2;
    float bottom = -top;
    Mat4::createOrthographicOffCenter(-width/2, width/2, -height/2, height/2, -1.0f, 1.0f, &pMat);
    
    setPosition(left, right, bottom, top);
}

void CSMatrixShader::onDraw() {
    glUniformMatrix4fv(getHandle(UNIFORM_MVPMATRIX), 1, false, mvpMatrix.m);
}

/**
 * Draw specify Texture use this Shader.
 *
 * @param texName Texture that need to be drawn
 * @param fbo Framebuffer object
 */
void CSOneInputShader::draw(const int texName, const CSFramebufferObject *fbo) {
    useProgram();
    
    glBindBuffer(GL_ARRAY_BUFFER, getVertexBufferName());
    glEnableVertexAttribArray(getHandle(DEFAULT_ATTRIB_POSITION));
    glVertexAttribPointer(getHandle(DEFAULT_ATTRIB_POSITION), VERTICES_DATA_POS_SIZE, GL_FLOAT, false, VERTICES_DATA_STRIDE_BYTES, (GLvoid*)VERTICES_DATA_POS_OFFSET);
    glEnableVertexAttribArray(getHandle(DEFAULT_ATTRIB_TEXTURE_COORDINATE));
    glVertexAttribPointer(getHandle(DEFAULT_ATTRIB_TEXTURE_COORDINATE), VERTICES_DATA_UV_SIZE, GL_FLOAT, false, VERTICES_DATA_STRIDE_BYTES, (GLvoid*)VERTICES_DATA_UV_OFFSET);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texName);
    glUniform1i(getHandle(DEFAULT_UNIFORM_SAMPLER), 0);
    
    onDraw();
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
    glDisableVertexAttribArray(getHandle(DEFAULT_ATTRIB_POSITION));
    glDisableVertexAttribArray(getHandle(DEFAULT_ATTRIB_TEXTURE_COORDINATE));
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
    
void CSFlipVerticalShader::initVerticesData() {
    const float VERTICES_DATA[] = {
        // X, Y, Z, U, V
        -1.0f,  1.0f, 0.0f, 0.0f, 0.0f, // 左上
        -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, // 左下
        1.0f,  1.0f, 0.0f, 1.0f, 0.0f, // 右上
        1.0f, -1.0f, 0.0f, 1.0f, 1.0f  // 右下
    };
    memcpy(verticesData, VERTICES_DATA, sizeof(VERTICES_DATA));
}

}
