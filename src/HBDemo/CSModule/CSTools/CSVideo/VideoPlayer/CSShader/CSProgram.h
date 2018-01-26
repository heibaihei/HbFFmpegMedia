//
//  CSProgram.h
//  Sample
//
//  Created by zj-db0519 on 2018/1/26.
//  Copyright © 2018年 meitu. All rights reserved.
//

#ifndef CSProgram_h
#define CSProgram_h

#include <stdio.h>
#include <string>
#include <unordered_map>
#include "CSImage.h"
#include "CSLog.h"

namespace HBMedia {

class CSProgramMaker;

typedef class CSProgram : public Ref {
public:
    CSProgram();
    ~CSProgram();
    
    bool link(CSProgramMaker* programMaker);
    
    bool isLinked();
    
    void use();
    
    /**
     * Get Shader Attribute or Uniform Location handle.
     *
     * @param name Attribute or Uniform string name.
     * @return Shader Location handle of the name.
     */
    const int getHandle(const std::string &name);
    
    void release();
    
private:
    int mProgram;
    
    /**
     * Map for saving Shader program resource id Handle.
     */
    std::unordered_map<std::string, int> mHandleMap;
} CSProgram;

}

#endif /* CSProgram_h */
