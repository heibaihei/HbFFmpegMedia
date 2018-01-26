//
//  CSGlUtil.h
//  Sample
//
//  Created by zj-db0519 on 2018/1/26.
//  Copyright © 2018年 meitu. All rights reserved.
//

#ifndef CSGlUtil_h
#define CSGlUtil_h

#include <stdio.h>
#include <string>
#include <OpenGL/gl.h>

#include "CSDefine.h"
#include "CSLog.h"

namespace HBMedia {

typedef class CSGlUtils {
private:
    CSGlUtils();
    ~CSGlUtils();
    
public:
    /**
     * Fail code for OpenGL operation<p>
     *
     * @see {@link #createProgram(String, String)}
     * @see {@link #loadShader(int, String)}
     */
    static const int INVALID;
    
    /**
     * Check whether last OpenGL API call is fail or not.
     *
     * @param op last OpenGL operation name.
     * @throws GLException If last operation was fail. Throw exception to notify caller.
     */
    static void checkGlError(const std::string &op);
    
    /**
     *
     *
     * @param vertexSource Vertex Shader Source code.
     * @param fragmentSource Fragment Shader Source code.
     * @return If success return OpenGL program ID, else return {@link #INVALID}
     * @throws GLException OpenGL API if OpenGL operation run fail!
     */
    static int createProgram(const std::string &vertexSource, const std::string &fragmentSource);
    
    /**
     * Create Shader program with VertexShader and FragmentShader.
     * time-consuming, 10ms in bad case
     *
     * @param vertexShader VertexShader OpenGL ID
     * @param pixelShader FragmentShader OpenGL ID
     * @return If success return OpenGL program ID, else return {@link #INVALID}
     * @throws GLException OpenGL API if OpenGL operation run fail!
     */
    static int createProgram(const int vertexShader, const int pixelShader);
    
    /**
     * Create specify Shader Type Shader with Source code.
     *
     * @param shaderType Shader Type Vertex/Fragment
     * @param source Shader code source string.
     * @return If success return OpenGL program ID, else return {@link #INVALID}
     * @see {@link GLES20#GL_VERTEX_SHADER}
     * @see {@link GLES20.GL_FRAGMENT_SHADER}
     */
    static int loadShader(const int shaderType, const std::string &source);
    
    /**
     * Generate OpenGL Buffer data with FloatBuffer.
     *
     * @param data That need send to OpenGL
     * @return OpenGL ID
     */
    static int createBuffer(const float data[],const int length);
    
    /**
     * Update OpenGL buffer data with FloatBuffer
     *
     * @param bufferName OpenGL buffer ID
     * @param data FloatBuffer data for update to OpenGL
     * @param lenght length of data buffer
     */
    static void updateBufferData(const int bufferName, const float data[], int length);
    
    //////////////////////////////////////////////////////////////////////////
    // Texture & Sampler
    
    /**
     * Set default texture parameter.
     *
     * @param target Texture target. GL_TEXTURE2D/OES
     * @param mag GL_TEXTURE_MAG_FILTER
     * @param min GL_TEXTURE_MIN_FILTER
     */
    static void setupSampler(const int target, const int mag, const int min);
    
private:
    static const int FLOAT_SIZE_BYTES;
} CSGlUtils;

}
#endif /* CSGlUtil_h */
