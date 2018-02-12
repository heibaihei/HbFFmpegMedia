//
//  CSMatrixShader.h
//  Sample
//
//  Created by zj-db0519 on 2018/1/26.
//  Copyright © 2018年 meitu. All rights reserved.
//

#ifndef CSMatrixShader_h
#define CSMatrixShader_h

#include <stdio.h>
#include <string>
#include "CSMathBase.h"
#include "CSShader.h"

namespace HBMedia {

typedef class CSProgramMaker CSProgramMaker;
typedef class CSFramebuffer CSFramebuffer;
    
typedef class CSOneInputShader : public CSShader {
public:
    CSOneInputShader(const std::string &vertexShaderSource = DEFAULT_VERTEX_SHADER, const std::string &fragmentShaderSource = DEFAULT_FRAGMENT_SHADER) : CSShader(vertexShaderSource, fragmentShaderSource) {}
    
    CSOneInputShader(CSProgramMaker *glProgramMaker) : CSShader(glProgramMaker) {}
    
    CSOneInputShader(const std::string &internalVertexShaderFile, bool vEncrypt, const std::string &internalFragmentShaderFile, bool fEncrypt) : CSShader(internalVertexShaderFile, vEncrypt, internalFragmentShaderFile, fEncrypt) {}
    
public:
    virtual void draw(const int texName, const CSFramebuffer *fbo);
} CSOneInputShader;
    
typedef class CSMatrixShader : public CSOneInputShader {
    
public:
    /**
     * Vertex shader code.
     */
    static const std::string MATRIX_VERTEX_SHADER;
    static const std::string UNIFORM_MVPMATRIX;    // mat4
    
    static const std::string MATRIX_ROTATE_X;
    static const std::string MATRIX_ROTATE_Y;
    static const std::string MATRIX_ROTATE_Z;
    static const std::string MATRIX_MOVE_X;
    static const std::string MATRIX_MOVE_Y;
    static const std::string MATRIX_SCALE_X;
    static const std::string MATRIX_SCALE_Y;
    
    /**
     *  useAlpha = true: keep alpha of texture
     *  useAlpha = false: set alpha = 1
     */
    CSMatrixShader(bool useAlpha = true);
    
    virtual ~CSMatrixShader(){}
    
    Mat4 &getMVPMatrix () {
        return mvpMatrix;
    }
    
    void setMVPMatrix (const Mat4 &mvp) {
        mvpMatrix.set(mvp);
    }
    
    void setProjection(float width, float height);
    
    virtual void setShaderData(const std::string& name, const void* data);
    
protected:
    virtual void onDraw();
    
private:
    Mat4 pMat;
    Mat4 mvpMatrix;
    
    static const std::string ALPHAONE_FRAGMENT_SHADER;
    
    const std::string& getFragmentShader(bool useAlpha);
} CSMatrixShader;

    
typedef class CSFlipVerticalShader :public CSOneInputShader {
public:
    CSFlipVerticalShader() {
        mShaderType = FLIP_SHADER;
        initVerticesData();
    }
    
    virtual ~CSFlipVerticalShader() {};
private:
    void initVerticesData();
} CSFlipVerticalShader;

}

#endif /* CSMatrixShader_h */
