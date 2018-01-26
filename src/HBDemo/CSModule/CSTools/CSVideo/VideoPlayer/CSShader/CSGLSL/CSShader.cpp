//
//  CSShader.cpp
//  Sample
//
//  Created by zj-db0519 on 2018/1/26.
//  Copyright © 2018年 meitu. All rights reserved.
//

#include "CSShader.h"
#include "CSGlUtil.h"
#include "CSProgram.h"
#include "CSTexture.h"
#include "CSTextureCache.h"
#include "CSShaderMaker.h"
#include "CSProgramMaker.h"
#include "CSProgramCache.h"

namespace HBMedia {
    
    const std::string CSShader::DEFAULT_ATTRIB_POSITION = "aPosition";
    
    const std::string CSShader::DEFAULT_ATTRIB_TEXTURE_COORDINATE = "aTextureCoord";
    
    const std::string CSShader::DEFAULT_UNIFORM_SAMPLER = "sTexture";
    
    const std::string CSShader::DEFAULT_UNIFORM_PERCENT = "uPercent";
    
    const std::string CSShader::DEFAULT_VERTEX_SHADER =
    "attribute vec4 aPosition;\n"
    "attribute vec4 aTextureCoord;\n"
    "varying vec2 vTextureCoord;\n"
    "void main() {\n"
    "gl_Position = aPosition;\n"
    "vTextureCoord = aTextureCoord.xy;\n"
    "}\n";
    
    const std::string CSShader::DEFAULT_FRAGMENT_SHADER =
    "#ifdef GL_ES\n"
    "precision mediump float;\n"    // specify float precision.
    "#else\n"
    "#define highp\n"
    "#endif\n"
    "varying highp vec2 vTextureCoord;\n"
    "uniform sampler2D sTexture;\n"
    "void main() {\n"
    "gl_FragColor = texture2D(sTexture, vTextureCoord);\n"
    "}\n";
    
    const float CSShader::VERTICES_DATA[] =  {
        // GL_TRIANGLE_STRIP 0,1,2;2,1,3
        // X, Y, Z, U, V
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f, // 左上
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, // 左下
        1.0f,  1.0f, 0.0f, 1.0f, 1.0f, // 右上
        1.0f, -1.0f, 0.0f, 1.0f, 0.0f  // 右下
    };
    
    CSShader::CSShader() {
        onDrawEasyingFunc = nullptr;
        mGLProgramMaker = NULL;
        mProgram = NULL;
        needUpdateBufferData = false;
        
        mixLast = false;
        useOriginalSprite = false;
        
        frameWidth = 0;
        frameHeight = 0;
        
        initVerticesData();
    }
    
    CSShader::CSShader(const std::string &vertexShaderSource, const std::string &fragmentShaderSource)
    :mVertexShaderSource(vertexShaderSource)
    ,mFragmentShaderSource(fragmentShaderSource)
    ,mProgram(NULL)
    ,mVertexBufferName(CSGlUtils::INVALID)
    ,mFirstInputTexture(nullptr)
    {
        onDrawEasyingFunc = nullptr;
        mGLProgramMaker = new CSProgramMaker(mVertexShaderSource, mFragmentShaderSource);
        needUpdateBufferData = false;
        
        mixLast = false;
        useOriginalSprite = false;
        
        frameWidth = 0;
        frameHeight = 0;
        
        initVerticesData();
    }
    
    CSShader::CSShader(CSProgramMaker *glProgramMaker)
    :mGLProgramMaker(glProgramMaker)
    ,mProgram(NULL)
    ,mVertexBufferName(CSGlUtils::INVALID)
    ,mFirstInputTexture(nullptr)
    {
        onDrawEasyingFunc = nullptr;
        needUpdateBufferData = false;
        
        mixLast = false;
        useOriginalSprite = false;
        
        frameWidth = 0;
        frameHeight = 0;
        
        initVerticesData();
    }
    
    CSShader::CSShader(const std::string &internalVertexShaderFile, bool vEncrypt, const std::string &internalFragmentShaderFile, bool fEncrypt)
    :mProgram(NULL)
    ,mVertexBufferName(CSGlUtils::INVALID)
    ,mFirstInputTexture(nullptr)
    {
        onDrawEasyingFunc = nullptr;
        CSShaderMaker* vShaderMaker = CSShaderMakerFactory::getShaderMaker(GL_VERTEX_SHADER, internalVertexShaderFile, true, vEncrypt);
        CSShaderMaker* fShaderMaker = CSShaderMakerFactory::getShaderMaker(GL_FRAGMENT_SHADER, internalFragmentShaderFile, true, fEncrypt);
        mGLProgramMaker = new CSProgramMaker(vShaderMaker, fShaderMaker);
        needUpdateBufferData = false;
        
        mixLast = false;
        useOriginalSprite = false;
        
        frameWidth = 0;
        frameHeight = 0;
        
        initVerticesData();
    }
    
    CSShader::~CSShader() {
        SAFE_DELETE(mGLProgramMaker);
        // Make sure shader's resource all release.
        release();
    }
    
    CSProgramMaker* CSShader::getGLProgramMaker() const {
        return mGLProgramMaker;
    }
    
    void CSShader::initVerticesData() {
        memcpy(verticesData, VERTICES_DATA, sizeof(VERTICES_DATA));
    }
    
    void CSShader::setup() {
        if (CSShader::isSetuped()) {
            if (needUpdateBufferData) {
                CSGlUtils::updateBufferData(mVertexBufferName, verticesData, sizeof(verticesData)/sizeof(verticesData[0]));
                needUpdateBufferData = false;
            }
            
            return;
        }
        
        mProgram = CSProgramCache::createProgram(mGLProgramMaker);
        
        mVertexBufferName = CSGlUtils::createBuffer(verticesData, sizeof(verticesData)/sizeof(verticesData[0]));
        
        needUpdateBufferData = false;
        
        if (mFirstInputTextureImage.get() != nullptr) {
            // TODO: 加载纹理
        }
    }
    
    
    void CSShader::release() {
        if (CSShader::isSetuped()) {
            CSProgramCache::releaseProgram(mProgram);
            mProgram = NULL;
            
            GLuint buffers[] = {static_cast<GLuint>(mVertexBufferName)};
            glDeleteBuffers(1, buffers);
            mVertexBufferName = 0;
        }
        frameWidth = 0;
        frameHeight = 0;
        
        //TODO: For now just set it to invalid. Maybe we should delete texture at this time.
        mFirstInputTexture = nullptr;
    }
    
    void CSShader::onDraw() {
        if (onDrawFunc) {
            onDrawFunc(this);
        }
        if (onDrawEasyingFunc) {
            onDrawEasyingFunc(this);
        }
    }
    
    void CSShader::useProgram() {
        mProgram->use();
    }
    
    const int CSShader::getHandle(const std::string &name) {
        return mProgram->getHandle(name);
    }
    
    bool CSShader::isSetuped() {
        return (mProgram != NULL && mProgram->isLinked());
    }
    
    bool CSShader::containType(ShaderType type) {
        return mShaderType == type;
    }
    
    void CSShader::setUV(float sU, float eU, float sV, float eV) {
        if (verticesData[3] != sU || verticesData[13] != eU ||
            verticesData[9] != sV || verticesData[4] != eV) {
            verticesData[3] = sU;
            verticesData[4] = eV;
            verticesData[8] = sU;
            verticesData[9] = sV;
            verticesData[13] = eU;
            verticesData[14] = eV;
            verticesData[18] = eU;
            verticesData[19] = sV;
            
            needUpdateBufferData = true;
        }
    }
    
    void CSShader::setPosition(float left, float right, float bottom, float top) {
        if (verticesData[0] != left || verticesData[10] != right ||
            verticesData[6] != bottom || verticesData[1] != top) {
            verticesData[0] = left;
            verticesData[1] = top;
            verticesData[5] = left;
            verticesData[6] = bottom;
            verticesData[10] = right;
            verticesData[11] = top;
            verticesData[15] = right;
            verticesData[16] = bottom;
            
            needUpdateBufferData = true;
        }
    }
    
    void CSShader::setShaderData(const std::string& name, const void* data) {
        
    }
    
    void CSShader::setMixLast(bool mix) {
        mixLast = mix;
    }
    
    bool CSShader::needMixLast() const {
        return mixLast;
    }
    
    void CSShader::setUseOriginalSprite(bool use) {
        useOriginalSprite = use;
    }
    
    bool CSShader::needUseOriginalSprite() const {
        return useOriginalSprite;
    }
    
    void CSShader::setFrameSize(const int width, const int height) {
        frameWidth = width;
        frameHeight = height;
    }
    
    void CSShader::setInputImageAtIndex(std::shared_ptr<CSImage> image, int texIndex)
    {
        mFirstInputTextureImage = image;
        if (nullptr != mFirstInputTexture) {
            CSTextureCache::releaseTexture((CSTexture*)mFirstInputTexture);
            mFirstInputTexture = nullptr;
        }
    }
    
    const std::string CSShader::getUniformPercentDesc() {
        return DEFAULT_UNIFORM_PERCENT;
    }
    
    void CSShader::setUniformFloat(const std::string &uniformName, float value)
    {
        glUniform1f(getHandle(uniformName), value);
    }
    
    void CSShader::setDrawEasyingFunc(std::function<void(CSShader*)> func) {
        onDrawEasyingFunc = func;
    }
    
    void CSShader::setDrawFunc(std::function<void(CSShader*)> func)
    {
        onDrawFunc = func;
    }
}
