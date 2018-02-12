//
//  CSShader.h
//  Sample
//
//  Created by zj-db0519 on 2018/1/26.
//  Copyright © 2018年 meitu. All rights reserved.
//

#ifndef CSShader_h
#define CSShader_h

#include <stdio.h>
#include <string>
#include <functional>
#include "CSImage.h"

namespace HBMedia {

    typedef class Texture Texture;
    typedef class CSProgram CSProgram;
    typedef class CSProgramMaker CSProgramMaker;
    typedef class CSFramebuffer CSFramebuffer;
    
enum ShaderType {
    UNKNOW_SHADER,
    DEFAULT_SHADER,
    MATRIX_SHADER,
    BRIGHTNESS_SHADER,
    CONTRAST_SHADER,
    SATURATION_SHADER,
    FLIP_SHADER,
    ADD_BLEND_SHADER,
    ALPHA_BLEND_SHADER,
    COLOR_BLEND_SHADER,
    MAPY_SHADER,
    MAPY512_SHADER,
    GERERNAL_MAP_SHADER,
    MASK_SHADER,
    GAUSS_V_SHADER,
    GAUSS_H_SHADER,
    GAUSS_SHADER,
    GAUSS_BGP_SHADER,
    FADE_SHADER,
    SCREEN_SHADER,
    PARALLEL_SHADER,
    MIX_INPUT_SHADER,
    MOVE_TO_SLOPE_SHADER,
    REMOVE_PART_SHADER,
    MOVE_TEX_MIX_SRC,
    DARKCORNER_SHADER,
    SOFTFOCUS_SHADER,
    SEGMENT_SHADER,
    SCALE_SHADER,
    DISPLACEMENT_SHADER,
    THREEDGLASSES_SHADER,
    SOFTFOCUS_USEMASK_SHADER,
    SKINBEAUTY_SHADER,
    MOTION_BLUR_SHADER,
    REPLACE_SHADER,
};

typedef class CSShader : public Ref {
    
public:
    /**
     * Default Vertex position parameter in shader
     */
    static const std::string DEFAULT_ATTRIB_POSITION;
    
    /**
     * Default Texture coordinate parameter name in shader.
     */
    static const std::string DEFAULT_ATTRIB_TEXTURE_COORDINATE;
    
    /**
     * Default Texture uniform sampler name in shader.
     */
    static const std::string DEFAULT_UNIFORM_SAMPLER;
    
    /**
     *  Uniform for percent control in fragmentshader.
     */
    static const std::string DEFAULT_UNIFORM_PERCENT;
    
    /**
     *  获取 Percent 对象脚本描述符，不同脚本重载以下接口实现描述符复用
     */
    virtual const std::string getUniformPercentDesc();
    
public:
    virtual ~CSShader();
    
    CSProgramMaker* getGLProgramMaker() const;
    
    /**
     * Actually create all GLSL resource that this object will need.
     */
    virtual void setup();
    
    /**
     * Set Shader Frame Size.
     * 设置该shader绘制在多大的fbo上，shadergroup和parallelshader感觉这个大小创建fbo，类似高斯模糊需要感觉该像素大小计算百分比
     *
     * @param width Frame width
     * @param height Frame height.
     */
    virtual void setFrameSize(const int width, const int height);
    
    /**
     针对带旋转角度视频，出现素材角度旋转的问题
     通过对基类设置旋转角度值，让所有的shader都有角度，可以让有素材的shader对素材进行调整，没素材的shader也不会有影响
     add by zhangkang 20171108
     @param rotation角度值 -90，90，180
     */
    void setRotation(int rotation){mRotation = rotation;};
    /**
     * Release All resource.
     */
    virtual void release();
    
    /**
     * Draw specify Texture use this Shader.
     *
     * @param texName Texture that need to be drawn
     * @param fbo Framebuffer object，目标fbo
     */
    virtual void draw(const int texName, const CSFramebuffer *fbo) = 0;
    
    // 判断是否包含该类型shader
    virtual bool containType(ShaderType type);
    
    // call before setup
    // 确定opengl的uv坐标
    virtual void setUV(float sU, float eU, float sV, float eV);
    
    // call before setup
    // 确定opengl顶点
    void setPosition(float left, float right, float bottom, float top);
    
    // 根据name来设置shader中属性的值，data可以是不同类型子类做解析，目前都为float
    virtual void setShaderData(const std::string& name, const void* data);
    
    // 未使用，可移除
    void setMixLast(bool mix);
    bool needMixLast() const;
    
    // 未使用，可移除
    void setUseOriginalSprite(bool use);
    bool needUseOriginalSprite() const;
    
    /**
     *  Set texture image to the specify texture index in shader.
     *
     *  @author Javan
     *  @since 2016.7.12
     *  @param image image for texture to load.
     *  @param texIndex texture index to be set.
     */
    virtual void setInputImageAtIndex(std::shared_ptr<CSImage> image, int texIndex);
    
    /**
     *  Set uniform value
     *
     *  @param uniformName uniform set for param name.
     *  @param value       value for uniform.
     */
    void setUniformFloat(const std::string &uniformName, float value);
    
    /**
     *  @func updateShaderAttributeWithTime
     *  @desc 根据外部时钟更新 shader 中的属性，主要应用场景：部分属性需要应用上缓动值
     *        由子类去派生实例化该接口
     *  @param clock 外部当前时钟
     */
    virtual void updateShaderAttributeWithTime(int64_t clock) { };
    
    /**
     *  Set onDraw lambda func.
     *
     *  @param func draw function for shader.
     */
    void setDrawFunc(std::function<void(CSShader*)> func);
    virtual void setDrawEasyingFunc(std::function<void(CSShader*)> func);
    
protected:
    CSShader();
    
    /**
     * Create A Shader program With provide Vertex And Fragment Shader Source Code.
     *
     * @param vertexShaderSource Vertex Shader Source Code
     * @param fragmentShaderSource Fragment Shader Source Code
     */
    CSShader(const std::string &vertexShaderSource, const std::string &fragmentShaderSource);
    
    /**
     * Create A shader program with custom {@link GLES20ProgramMaker}.
     * @param glProgramMaker Custom OpenGL Shader program maker.
     */
    CSShader(CSProgramMaker *glProgramMaker);
    
    CSShader(const std::string &internalVertexShaderFile, bool vEncrypt, const std::string &internalFragmentShaderFile, bool fEncrypt);
    
    /**
     * Append some more draw actions.
     */
    virtual void onDraw();
    
    void useProgram();
    
    /**
     * Get VertexBuffer OpenGL ID.
     *
     * @return VertexBuffer ID, No created return {@code 0}
     */
    const int getVertexBufferName() {
        return mVertexBufferName;
    }
    
    /**
     * Get Shader Attribute or Uniform Location handle.
     *
     * @param name Attribute or Uniform string name.
     * @return Shader Location handle of the name.
     */
    const int getHandle(const std::string &name);
    
protected:
    // 原视频的角度值
    int mRotation;
    int frameWidth;
    int frameHeight;
    
    // need mix with last
    bool mixLast;
    // need draw use texture of original sprite
    bool useOriginalSprite;
    
    float verticesData[20];
    bool needUpdateBufferData;
    
    ShaderType mShaderType;
    
    /**
     * Default Vertex Shader.
     */
    static const std::string DEFAULT_VERTEX_SHADER;
    
    /**
     * Default fragment shader.
     */
    static const std::string DEFAULT_FRAGMENT_SHADER;
    
    static const int FLOAT_SIZE_BYTES = 4;
    static const int VERTICES_DATA_POS_SIZE = 3;
    static const int VERTICES_DATA_UV_SIZE = 2;
    static const int VERTICES_DATA_STRIDE_BYTES = (VERTICES_DATA_POS_SIZE + VERTICES_DATA_UV_SIZE) * FLOAT_SIZE_BYTES;
    static const int VERTICES_DATA_POS_OFFSET = 0 * FLOAT_SIZE_BYTES;
    static const int VERTICES_DATA_UV_OFFSET = VERTICES_DATA_POS_OFFSET + VERTICES_DATA_POS_SIZE * FLOAT_SIZE_BYTES;
    
    virtual bool isSetuped ();
    
protected:
    /**
     *  The first texture that be set at index zero.
     *  @author Javan
     */
    Texture *mFirstInputTexture;
    std::shared_ptr<CSImage> mFirstInputTextureImage;
    
private:
    
    void initVerticesData();
    
    /**
     * Vertices and ST/UV Vertices.
     * 包含定点坐标和纹理UV坐标的数组
     */
    static const float VERTICES_DATA[];
    
    
    /**
     * Vertex Shader Source Code.
     */
    const std::string mVertexShaderSource;
    
    /**
     * Fragment Shader Source Code.
     */
    const std::string mFragmentShaderSource;
    
    /**
     * OpenGL Program Maker, For some situation shader code maybe be not a String.
     */
    CSProgramMaker *mGLProgramMaker;
    
    /**
     * Shader program
     */
    CSProgram* mProgram;
    
    /**
     * Vertex Buffer OpenGL id
     */
    int mVertexBufferName;
    
    std::function<void(CSShader*)> onDrawFunc;
    std::function<void(CSShader*)> onDrawEasyingFunc;
    
} CSShader;
    
}


#endif /* CSShader_h */
