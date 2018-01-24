//
//  CSMathHelper.h
//  mtmv
//
//  Created by cwq on 16/5/20.
//  Copyright © 2016年 meitu. All rights reserved.
//

#ifndef CSMathHelper_h
#define CSMathHelper_h

#include <string>
#include <vector>
#include "CSVec2.h"
#include "CSVec4.h"

class MathHelper {
public:
    
    static std::string getDirectoryPath(const std::string& filePath);
    
    static std::string intToString(int i);
    
    static int stringToInt(const std::string& sInt);
    
    static float stringToFloat(const std::string& sFloat);
    
    /**
     *  transform string{x,y} to Vec2(x,y)
     *
     *  @param sVec {x,y}
     *
     *  @return Vec2
     */
    static Vec2 stringToVec2(const std::string& sVec);
    
    /**
     *  transform string{x,y,z,w} to Vec2(x,y,z,w)
     *
     *  @param sVec {x,y,y,z}
     *
     *  @return Vec4
     */
    static Vec4 stringToVec4(const std::string &sVec);
    
    static std::vector<std::string> stringToVec2String(const std::string& sVec);
    
    /**
     *  transform string"x'delim'y" to Vec2(x,y)
     *  if no delim: Vec2(0,y)
     *  if no y: Vec2(x,0)
     *
     *  @param str   "x'delim'y"
     *  @param delim delimiter
     *
     *  @return Vec2
     */
    static Vec2 stringToVec2ByDelimiter(const std::string& str, const char* delim);
    
    /**
     *  transform string{aw+b,ch+d} to Vec2(a,c), Vec2(b, d)
     *  the first Vec2 is percent of w or h
     *  the second Vec2 is pixel
     *
     *  @param sVec {aw+b,ch+d}
     *
     *  @return two Vec2 in vector
     */
    static std::vector<Vec2> stringToTwoVec2ByWH(const std::string& sVec);
    
    // split by delim
    static std::vector<std::string> splitString(const std::string& str, const char* delim);
    
private:
    MathHelper();
};

#endif /* CSMathHelper_h */
