//
//  CSShaderGroup.cpp
//  Sample
//
//  Created by zj-db0519 on 2018/2/12.
//  Copyright © 2018年 meitu. All rights reserved.
//

#include "CSShaderGroup.h"

namespace HBMedia {
    
CSShaderGroup::CSShaderGroup() {
    mSetuped = false;
}

CSShaderGroup::CSShaderGroup(std::list<CSShader*> shaders) {
    mSetuped = false;
    
    std::list<CSShader*>::iterator it = shaders.begin();
    for (; it!=shaders.end(); ++it) {
        if (*it)
            mShaders.push_back(*it);
    }
}

CSShaderGroup::~CSShaderGroup() {
    release();
    
    for (auto &&shader : mShaders) {
        if (nullptr != shader)
            shader->decreaseRef();
    }
    
    mShaders.clear();
}

void CSShaderGroup::addShader(CSShader* shader) {
    if (shader) {
        mShaders.push_back(shader);
    }
}

bool CSShaderGroup::isSetuped() {
    return mSetuped;
}

void CSShaderGroup::setup() {
    for (CSShader *shader : mShaders) {
        if (nullptr != shader) shader->setup();
    }
    
    if (isSetuped()) {
        return;
    }
    
    if (!mShaders.empty()) {
        // 替换目前多个，改成只需要2个fbo就行
        //        const int max = (int)mShaders.size();
        //        int count = 0;
        //        for (GLES20Shader *shader : mShaders) {
        //            if (shader != nullptr) {
        //                GLES20FramebufferObject *fbo;
        //                if ((count + 1) < max) {
        //                    fbo = new GLES20FramebufferObject();
        //                } else {
        //                    fbo = nullptr;
        //                }
        //                mList.push_back(std::make_pair(shader, fbo));
        //                count++;
        //            }
        //        }
    }
    
    defaultShader.setup();
    
    mSetuped = true;
}

void CSShaderGroup::release() {
    if (!isSetuped()) {
        return;
    }
    //    for (const std::pair<GLES20Shader*, GLES20FramebufferObject*> pair : mList) {
    //        if (pair.first != nullptr) {
    //            pair.first->release();
    //        }
    //        if (pair.second != nullptr) {
    //            pair.second->release();
    //            delete (pair.second);
    //        }
    //    }
    //    mList.clear();
    
    for (auto &&shader : mShaders) {
        if (nullptr != shader) {
            shader->release();
        }
    }
    
    int count = sizeof(framebufferObjects)/ sizeof(framebufferObjects[0]);
    for (int i = 0; i != count; ++i) {
        framebufferObjects[i].release();
    }
    
    defaultShader.release();
    
    CSShader::release();
    
    mSetuped = false;
}

void CSShaderGroup::setFrameSize(const int width, const int height) {
    if (frameHeight == height && frameWidth == width) {
        return;
    }
    
    CSShader::setFrameSize(width, height);
    
    //    for (const std::pair<GLES20Shader*, GLES20FramebufferObject*> pair : mList) {
    //        if (pair.first != nullptr) {
    //            pair.first->setFrameSize(width, height);
    //        }
    //        if (pair.second != nullptr) {
    //            pair.second->setup(width, height);
    //        }
    //    }
    
    for (auto &&shader : mShaders) {
        if (nullptr != shader) {
            shader->setFrameSize(width, height);
        }
    }
    int count = sizeof(framebufferObjects)/ sizeof(framebufferObjects[0]);
    for (int i = 0; i != count; ++i) {
        framebufferObjects[i].setup(width, height);
    }
    
    defaultShader.setFrameSize(width, height);
}

void CSShaderGroup::setDrawEasyingFunc(std::function<void(CSShader*)> func)
{
    for (auto it = mShaders.begin(); it != mShaders.end(); ++it) {
        CSShader* shader = *it;
        if (nullptr != shader) {
            shader->setDrawEasyingFunc(func);
        }
    }
}

void CSShaderGroup::draw(const int texName, const CSFrameBuffer *fbo) {
    int prevTexName = texName;
    int w_index = 0;    // 循环利用两个fbo
    int fbo_count = sizeof(framebufferObjects)/ sizeof(framebufferObjects[0]);
    int shader_count = static_cast<int>(mShaders.size()), i = 0;
    CSShader *lastShader = nullptr;
    
    // If there is no shader in group;
    if (mShaders.size() == 0) {
        goto final;
    }
    
    for (auto &&shader : mShaders) {
        if (shader == nullptr) {
            ++i;
            continue;
        }
        if ((i + 1) != shader_count) {
            // 不到倒数第一个shader
            CSFrameBuffer &localFbo = framebufferObjects[w_index++%fbo_count];
            localFbo.enable();
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            // 给ShaderGroup里面的每一个shader都设置视频的旋转角度add by zhangkang 20171108
            shader->setRotation(mRotation);
            shader->draw(prevTexName, &localFbo);
            prevTexName = localFbo.getTexName();
            ++i;
        } else {
            break;
        }
    }
    
    lastShader = mShaders.back();
    
    final:
    if (nullptr != fbo) {
        fbo->enable();
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    
    if (lastShader != nullptr) {
        // 给ShaderGroup里面的最后一个shader都设置视频的旋转角度，add by zhangkang 20171108
        lastShader->setRotation(mRotation);
        lastShader->draw(prevTexName, fbo);
    } else {
        defaultShader.draw(prevTexName, fbo);
    }
    
}

bool CSShaderGroup::containType(ShaderType type) {
    if (CSShader::containType(type))
        return true;
    
    for (CSShader *shader : mShaders) {
        if (nullptr != shader && shader->containType(type)) {
            return true;
        }
    }
    return false;
}

void CSShaderGroup::setUV(float sU, float eU, float sV, float eV) {
    // just change first shader uv
    auto it = mShaders.begin();
    if (it != mShaders.end()) {
        (*it)->setUV(sU, eU, sV, eV);
    }
}

void CSShaderGroup::setShaderData(const std::string& name, const void* data) {
    for (CSShader *shader : mShaders) {
        if (nullptr != shader) {
            shader->setShaderData(name, data);
        }
    }
}

CSShader *CSShaderGroup::getShaderAtOrder(int orderIndex) {
    if (orderIndex > mShaders.size() || mShaders.size() == 0) {
        return nullptr;
    }
    return mShaders[orderIndex];
}

void CSShaderGroup::AddOrReplaceShaderAtOrder(int orderIndex, CSShader *shader) {
    if (mShaders.size() <= orderIndex) {
        // 不够的时候再次开大
        mShaders.resize(static_cast<unsigned long>(orderIndex+1));
    }
    
    if (mShaders[orderIndex] && shader != mShaders[orderIndex]) {
        // 已经存在了.
        mShaders[orderIndex]->release();
        mShaders[orderIndex]->decreaseRef();
        mShaders[orderIndex] = nullptr;
    }
    
    if (mShaders[orderIndex] == nullptr) {
        if (shader != nullptr) {
            shader->setup();
            shader->setFrameSize(frameWidth, frameHeight);
        }
        mShaders[orderIndex] = shader;
    }
}

}
