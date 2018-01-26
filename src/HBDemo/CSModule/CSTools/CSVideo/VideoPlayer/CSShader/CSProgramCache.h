//
//  CSProgramCache.h
//  Sample
//
//  Created by zj-db0519 on 2018/1/26.
//  Copyright © 2018年 meitu. All rights reserved.
//

#ifndef CSProgramCache_h
#define CSProgramCache_h

#include <stdio.h>
#include <unordered_map>

namespace HBMedia {
    
class CSProgramMaker;
class CSProgram;

typedef class CSProgramCache {
public:
    static CSProgram* createProgram(CSProgramMaker* programMaker);
    
    static void releaseProgram(CSProgram* program);
    
    static void releaseAllPrograms();
    
    static void loadDefaultGLPrograms();
    
private:
    CSProgramCache();
    
    static bool defaultLoaded;
    
    static std::unordered_map<std::string, CSProgram*> _programs;
} CSProgramCache;
    
}

#endif /* CSProgramCache_h */
