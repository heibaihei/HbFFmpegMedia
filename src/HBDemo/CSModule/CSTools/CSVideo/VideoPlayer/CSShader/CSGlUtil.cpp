//
//  CSGlUtil.cpp
//  Sample
//
//  Created by zj-db0519 on 2018/1/26.
//  Copyright © 2018年 meitu. All rights reserved.
//

#include "CSGlUtil.h"

namespace HBMedia {
const int CSGlUtils::INVALID = 0;
const int CSGlUtils::FLOAT_SIZE_BYTES = 4;

CSGlUtils::CSGlUtils() {
    
}

CSGlUtils::~CSGlUtils() {
    
}
    
void CSGlUtils::checkGlError(const std::string &op) {
    int error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        LOGE("%s : glError  %d", op.c_str(), error);
    }
}

int CSGlUtils::createProgram(const std::string &vertexSource, const std::string &fragmentSource)
{
    // Load and generate Vertex Shader.
    const int vertexShader = loadShader(GL_VERTEX_SHADER, vertexSource);
    // Load and generate Fragment Shader.
    const int pixelShader = loadShader(GL_FRAGMENT_SHADER, fragmentSource);
    
    return createProgram(vertexShader, pixelShader);
}

int CSGlUtils::createProgram(const int vertexShader, const int pixelShader) {
    int program = glCreateProgram();
    
    if (program == CSGlUtils::INVALID) {
        LOGE("Could not create program");
        return program;
    }
    
    glAttachShader(program, vertexShader);
    glAttachShader(program, pixelShader);
    
    // time-consuming, 10ms in bad case
    glLinkProgram(program);
    int linkStatus[1] = {0};
    glGetProgramiv(program, GL_LINK_STATUS, linkStatus);
    if (linkStatus[0] != GL_TRUE) {
        char log[512] = {0};
        glGetProgramInfoLog(program,512, NULL, log);
        LOGE("Could not link program: ");
        LOGE("%s",log);
        glDeleteProgram(program);
        program = CSGlUtils::INVALID;
        LOGE("Could not link program");
    }
    
    return program;
}

int CSGlUtils::loadShader(const int shaderType, const std::string &source) {
    int shader = glCreateShader(shaderType);
    if (shader == CSGlUtils::INVALID) {
        LOGE("Could not create shader %d : %s", shaderType, source.c_str());
        return shader;
    }
    
    const GLchar* pCode = source.c_str();
    GLint length = (GLint)source.length();
    
    glShaderSource(shader, 1, &pCode, &length);
    glCompileShader(shader);
    int compiled[] = {0};
    glGetShaderiv(shader, GL_COMPILE_STATUS, compiled);
    if (compiled[0] == 0) {
        char log[512] = {0};
        glGetShaderInfoLog(shader,512, NULL, log);
        LOGE("Could not compile shader %d", shaderType);
        LOGE("%s",log);
        glDeleteShader(shader);
        shader = CSGlUtils::INVALID;
    }
    
    return shader;
}

int CSGlUtils::createBuffer(const float data[],const int length) {
    GLuint buffers[] = {0};
    glGenBuffers(1, buffers);
    updateBufferData(buffers[0], data, length);
    return buffers[0];
}
    
void CSGlUtils::updateBufferData(const int bufferName, const float data[], int length) {
    glBindBuffer(GL_ARRAY_BUFFER, bufferName);
    glBufferData(GL_ARRAY_BUFFER, length * FLOAT_SIZE_BYTES, data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
    
void CSGlUtils::setupSampler(const int target, const int mag, const int min) {
    // Set Texture scale
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, mag);        // enlarge texture use mag filter
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, min);        // reduce texture use min filter
    // Set Texture wrap.
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

}
