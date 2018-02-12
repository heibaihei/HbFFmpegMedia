//
//  CSShaderGroup.h
//  Sample
//
//  Created by zj-db0519 on 2018/2/12.
//  Copyright © 2018年 meitu. All rights reserved.
//

#ifndef CSShaderGroup_h
#define CSShaderGroup_h

#include <stdio.h>
#include <list>
#include <vector>

#include "CSShader.h"
#include "CSFrameBuffer.h"
#include "CSMatrixShader.h"

namespace HBMedia {
// 串行shader，in->1->2->3->out
typedef class CSShaderGroup : public CSShader {
public:
    CSShaderGroup();
    
    CSShaderGroup(std::list<CSShader*> shaders);
    virtual ~CSShaderGroup();
    
    /**
     *  must call before setup
     *
     *  @param shader shader for draw
     */
    void addShader(CSShader* shader);
    
    virtual void setup() override;
    virtual void release() override;
    
    virtual void setFrameSize(const int width, const int height) override;
    virtual void draw(const int texName, const CSFrameBuffer *fbo);
    
    virtual bool containType(ShaderType type) override;
    
    virtual void setUV(float sU, float eU, float sV, float eV) override;
    
    virtual void setShaderData(const std::string& name, const void* data) override;
    
    virtual void setDrawEasyingFunc(std::function<void(CSShader*)> func);
    
    /**
     * 获取指定位置的shader，也可以理解为Shader的z_order
     *
     * @param orderIndex order的索引
     * @return shader at orderIndex, null for not shader.
     */
    CSShader *getShaderAtOrder(int orderIndex);
    void AddOrReplaceShaderAtOrder(int orderIndex, CSShader *shader);
protected:
    virtual bool isSetuped();
    
    //private:
    CSFrameBuffer framebufferObjects[2];
    std::vector<CSShader*> mShaders;
    CSOneInputShader defaultShader;
    bool mSetuped;
} CSShaderGroup;

}
#endif /* CSShaderGroup_h */
