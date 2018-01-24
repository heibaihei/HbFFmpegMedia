//
//  MathHelper.cpp
//  mtmv
//
//  Created by cwq on 16/5/20.
//  Copyright © 2016年 meitu. All rights reserved.
//

#include "CSMathHelper.h"
#include <sstream>
#include <stdlib.h>

std::string MathHelper::getDirectoryPath(const std::string& filePath) {
    return filePath.substr(0, filePath.find_last_of("/") + 1);
}

int MathHelper::stringToInt(const std::string& sInt) {
    return atoi(sInt.c_str());
}

std::string MathHelper::intToString(int i) {
    std::stringstream ret;
    ret << i;
    return ret.str();
}

float MathHelper::stringToFloat(const std::string& sFloat) {
    return atof(sFloat.c_str());
}

Vec2 MathHelper::stringToVec2(const std::string& sVec) {
    std::size_t found = sVec.find(",");
    std::string sX = sVec.substr(1, found - 1);
    std::string sY = sVec.substr(found + 1, sVec.size() - 1 - found - 1);
    return Vec2(atof(sX.c_str()), atof(sY.c_str()));
}

Vec4 MathHelper::stringToVec4(const std::string &sVec){
    std::string str(sVec);
    std::size_t found = str.find(",");
    std::string sX = str.substr(1, found - 1);
    str = str.substr(found+1);
    
    found = str.find(",");
    std::string sY = str.substr(0, found);
    str = str.substr(found+1);
    
    found = str.find(",");
    std::string sZ = str.substr(0, found);
    std::string sW = str.substr(found + 1, sVec.size() - 1 - found - 1);
    
    return Vec4(atof(sX.c_str()), atof(sY.c_str()), atof(sZ.c_str()), atof(sW.c_str()));
}

std::vector<std::string> MathHelper::stringToVec2String(const std::string& sVec) {
    std::size_t found = sVec.find(",");
    std::string sX = sVec.substr(1, found - 1);
    std::string sY = sVec.substr(found + 1, sVec.size() - 1 - found - 1);
    
    std::vector<std::string> strings;
    strings.reserve(2);
    strings.push_back(sX);
    strings.push_back(sY);
    return strings;
}

Vec2 MathHelper::stringToVec2ByDelimiter(const std::string& str, const char* delim) {
    Vec2 vec;
    std::size_t found = str.find(delim);
    if (found == std::string::npos) {
        // no delim, just y
        vec.y = atof(str.c_str());
    } else {
        std::string sX = str.substr(0, found);
        vec.x = atof(sX.c_str());
        if (found != (str.length() - 1)) {
            // after delim has y
            std::string sY = str.substr(found + 1, str.size() - 1 - found);
            vec.y = atof(sY.c_str());
        }
    }
    return vec;
}

std::vector<Vec2> MathHelper::stringToTwoVec2ByWH(const std::string& sVec) {
    std::size_t found = sVec.find(",");
    std::string sX = sVec.substr(1, found - 1);
    std::string sY = sVec.substr(found + 1, sVec.size() - 1 - found - 1);
    
    Vec2 first = MathHelper::stringToVec2ByDelimiter(sX, "w");
    Vec2 second = MathHelper::stringToVec2ByDelimiter(sY, "h");
    
    Vec2 percent(first.x, second.x);
    Vec2 pixel(first.y, second.y);
    
    std::vector<Vec2> vecs;
    vecs.reserve(2);
    vecs.push_back(percent);
    vecs.push_back(pixel);
    return vecs;
}

std::vector<std::string> MathHelper::splitString(const std::string& str, const char* delim) {
    std::vector<std::string> strings;
    
    char cStr[256];
    size_t length = str.length();
    str.copy(cStr, length);
    cStr[length] = '\0';
    
    char* pch;
    pch = strtok(cStr, delim);
    while (pch != NULL) {
        strings.push_back(std::string(pch));
        pch = strtok(NULL, delim);
    }
    
    return strings;
}
