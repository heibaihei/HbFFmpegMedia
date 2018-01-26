//
//  CSShaderMaker.hpp
//  Sample
//
//  Created by zj-db0519 on 2018/1/26.
//  Copyright © 2018年 meitu. All rights reserved.
//

#ifndef CSShaderMaker_h
#define CSShaderMaker_h

#include <stdio.h>
#include <string>

namespace HBMedia {

typedef class CSShaderMaker {
public:
    virtual ~CSShaderMaker(){};
    
    const std::string& getShaderSource() const;
    
    /**
     * 获取Maker创建的Shader的类型
     * @return 返回类型对应的id值
     */
    int getShaderType() {
        return mShaderType;
    }
    
    /**
     * 获取Shader在OpenGL 里面的 id 值
     *
     * @return 0 表示没有创建成功，大于0表示成功。
     */
    int getShader() {
        return mShader;
    }
    
    /**
     * 初始化Shader，也是创建Shader。
     * @return 返回 true 表示成功，false 表示失败。
     */
    bool init();
    
    /**
     *  释放申请的OpenGL资源
     */
    void release();
    
protected:
    CSShaderMaker(int shaderType) : mShaderType(shaderType) ,mShader(0) {};
    
    /**
     * 脚本类型 {@code GL_VERTEX_SHADER} or {@code GL_FRAGMENT_SHADER}
     */
    const int mShaderType;
    
    /**
     * Shader source in OpenGL
     */
    int mShader;
    
    std::string mShaderSource;
} CSShaderMaker;

typedef class CSStringShaderMaker : public CSShaderMaker {
public:
    CSStringShaderMaker(int shaderType, const std::string &shaderSource);
} CSStringShaderMaker;

}
#endif /* CSShaderMaker_h */
