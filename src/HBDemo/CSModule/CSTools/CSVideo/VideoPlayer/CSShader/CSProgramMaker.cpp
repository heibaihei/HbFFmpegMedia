//
//  CSProgramMaker.cpp
//  Sample
//
//  Created by zj-db0519 on 2018/1/26.
//  Copyright © 2018年 meitu. All rights reserved.
//

#include "CSProgramMaker.h"
#include <OpenGL/gl.h>
#include "CSDefine.h"
#include "CSGlUtil.h"
#include "CSShaderMaker.h"

namespace HBMedia {

CSProgramMaker::CSProgramMaker(const std::string &vertexShaderSource, const std::string &fragmentShaderSource)
:mVertexShaderMaker(nullptr)
,mFragmentShaderMaker(nullptr) {
    mVertexShaderMaker = new CSStringShaderMaker(GL_VERTEX_SHADER, vertexShaderSource);
    mFragmentShaderMaker = new CSStringShaderMaker(GL_FRAGMENT_SHADER, fragmentShaderSource);
}

CSProgramMaker::CSProgramMaker(CSShaderMaker *vertexShaderMaker, CSShaderMaker *fragmentShaderMaker)
:mVertexShaderMaker(vertexShaderMaker)
,mFragmentShaderMaker(fragmentShaderMaker) {
    
}

CSProgramMaker::~CSProgramMaker() {
    SAFE_DELETE(mVertexShaderMaker);
    SAFE_DELETE(mFragmentShaderMaker);
}

std::string CSProgramMaker::getProgrameKey() const {
    return mVertexShaderMaker->getShaderSource() + mFragmentShaderMaker->getShaderSource();
}

int CSProgramMaker::link()
{
    
    int vertexShader     = mVertexShaderMaker->init() ? mVertexShaderMaker->getShader() : 0;
    int fragmentShader   = mFragmentShaderMaker->init() ? mFragmentShaderMaker->getShader() : 0;
    
    int program = CSGlUtils::createProgram(vertexShader, fragmentShader);
    mVertexShaderMaker->release();
    mFragmentShaderMaker->release();
    
    return program;
}

}
