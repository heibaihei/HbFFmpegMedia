//
//  CSProgramMaker.h
//  Sample
//
//  Created by zj-db0519 on 2018/1/26.
//  Copyright © 2018年 meitu. All rights reserved.
//

#ifndef CSProgramMaker_h
#define CSProgramMaker_h

#include <stdio.h>
#include <string>

namespace HBMedia {

class CSShaderMaker;

class CSProgramMaker {
    
public:
    CSProgramMaker(const std::string &vertexShaderSource, const std::string &fragmentShaderSource);
    
    CSProgramMaker(CSShaderMaker *vertexShaderMaker, CSShaderMaker *fragmentShaderMaker);
    
    virtual ~CSProgramMaker();
    
    /**
     *  use vertexShader+fragmentShader as programeKey
     *
     *  @return programe key
     */
    std::string getProgrameKey() const;
    
    /**
     * Link OpenGL Vertex Shader and Fragment Shader
     *
     * @return OpenGL program id.
     */
    int link();
private:
    /**
     * Vertex Shader Maker
     */
    CSShaderMaker *mVertexShaderMaker;
    
    /**
     * Fragment Shader Source Code.
     */
    CSShaderMaker *mFragmentShaderMaker;
};

}
#endif /* CSProgramMaker_h */
