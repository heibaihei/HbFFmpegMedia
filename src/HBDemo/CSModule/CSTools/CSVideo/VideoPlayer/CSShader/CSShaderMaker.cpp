//
//  CSShaderMaker.cpp
//  Sample
//
//  Created by zj-db0519 on 2018/1/26.
//  Copyright © 2018年 meitu. All rights reserved.
//

#include "CSShaderMaker.h"
#include "CSGlUtil.h"

namespace HBMedia {
    
const std::string& CSShaderMaker::getShaderSource() const {
    return mShaderSource;
}

bool CSShaderMaker::init() {
    mShader = CSGlUtils::loadShader(mShaderType, mShaderSource);
    return mShader != 0;
}

void CSShaderMaker::release() {
    glDeleteShader(mShader);
}
    
CSStringShaderMaker::CSStringShaderMaker(int shaderType, const std::string &shaderSource)
: CSShaderMaker(shaderType) {
    mShaderSource = shaderSource;
}

}
