//
//  CSProgramCache.cpp
//  Sample
//
//  Created by zj-db0519 on 2018/1/26.
//  Copyright © 2018年 meitu. All rights reserved.
//

#include "CSProgramCache.h"
#include "CSProgram.h"
#include "CSProgramMaker.h"

namespace HBMedia {

bool CSProgramCache::defaultLoaded = false;
std::unordered_map<std::string, CSProgram*> CSProgramCache::_programs;

CSProgram* CSProgramCache::createProgram(CSProgramMaker* programMaker) {
    CSProgram* program = NULL;
    auto programeKey = programMaker->getProgrameKey();
    auto it = _programs.find(programeKey);
    if (it != _programs.end()) {
        program = it->second;
        
        if (program) {
            program->increaseRef();
        }
    } else {
        program = new CSProgram();
        if (program && program->link(programMaker)) {
            _programs.insert(std::make_pair(programeKey, program));
        } else {
            LOGE("GLES20ProgramCache::createProgram error!");
            SAFE_DELETE(program);
        }
    }
    
    return program;
}

void CSProgramCache::releaseProgram(CSProgram* program) {
    // TODO: need release immediately ?
    if (program) {
        // TODO: remove from cache, use key ?
        if (program->getRef() == 1) {
            
            program->release();
            
            for (auto it = _programs.begin(); it != _programs.end(); ++it) {
                if (it->second == program) {
                    _programs.erase(it);
                    break;
                }
            }
        }
        
        program->decreaseRef();
    }
}

void CSProgramCache::releaseAllPrograms() {
    CSProgram* program = NULL;
    for (auto it = _programs.begin(); it != _programs.end(); ++it) {
        program = it->second;
        program->release();
        SAFE_DELETE(program);
    }
    _programs.clear();
    defaultLoaded = false;
}

void CSProgramCache::loadDefaultGLPrograms() {
    if (defaultLoaded) {
        return;
    }
    
    defaultLoaded = true;
}

}
