//
//  CSProgram.cpp
//  Sample
//
//  Created by zj-db0519 on 2018/1/26.
//  Copyright © 2018年 meitu. All rights reserved.
//

#include "CSProgram.h"
#include "CSGlUtil.h"
#include "CSProgramMaker.h"

namespace HBMedia {

    CSProgram::CSProgram() {
        
    }
    
    CSProgram::~CSProgram() {
        mHandleMap.clear();
    }
    
    bool CSProgram::link(CSProgramMaker* programMaker) {
        mProgram = programMaker->link();
        return isLinked();
    }
    
    bool CSProgram::isLinked() {
        return mProgram != CSGlUtils::INVALID;
    }
    
    void CSProgram::use() {
        glUseProgram(mProgram);
    }
    
    const int CSProgram::getHandle(const std::string &name) {
        auto it = mHandleMap.find(name);
        if (it != mHandleMap.end()) {
            return it->second;
        }
        
        int location = glGetAttribLocation(mProgram, name.c_str());
        if (location == -1) {
            location = glGetUniformLocation(mProgram, name.c_str());
        }
        if (location == -1) {
            LOGV("Could not get attrib or uniform location for %s", name.c_str());
            return -1;
        }
        
        mHandleMap.insert(std::make_pair(name, location));
        return location;
    }
    
    void CSProgram::release() {
        if (isLinked()) {
            glDeleteProgram(mProgram);
            mProgram = CSGlUtils::INVALID;
            
            mHandleMap.clear();
        }
    }

}
