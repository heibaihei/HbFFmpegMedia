#ifndef CS_TYPES_H
#define CS_TYPES_H

#include <string>
#include "CSGeometry.h"
#include <OpenGL/gl.h>

NS_GLX_BEGIN

struct Color4B;
struct Color4F;

/** RGB color composed of bytes 3 bytes
 @since v3.0
 */
struct Color3B
{
    Color3B();
    Color3B(GLubyte _r, GLubyte _g, GLubyte _b);
    explicit Color3B(const Color4B& color);
    explicit Color3B(const Color4F& color);
    
    bool operator==(const Color3B& right) const;
    bool operator==(const Color4B& right) const;
    bool operator==(const Color4F& right) const;
    bool operator!=(const Color3B& right) const;
    bool operator!=(const Color4B& right) const;
    bool operator!=(const Color4F& right) const;
    
    bool equals(const Color3B& other) const
    {
        return (*this == other);
    }
    
    GLubyte r;
    GLubyte g;
    GLubyte b;
    
    static const Color3B WHITE;
    static const Color3B YELLOW;
    static const Color3B BLUE;
    static const Color3B GREEN;
    static const Color3B RED;
    static const Color3B MAGENTA;
    static const Color3B BLACK;
    static const Color3B ORANGE;
    static const Color3B GRAY;
};

/** RGBA color composed of 4 bytes
 @since v3.0
 */
struct Color4B
{
    Color4B();
    Color4B(GLubyte _r, GLubyte _g, GLubyte _b, GLubyte _a);
    explicit Color4B(const Color3B& color);
    explicit Color4B(const Color4F& color);
    
    bool operator==(const Color4B& right) const;
    bool operator==(const Color3B& right) const;
    bool operator==(const Color4F& right) const;
    bool operator!=(const Color4B& right) const;
    bool operator!=(const Color3B& right) const;
    bool operator!=(const Color4F& right) const;
    
    GLubyte r;
    GLubyte g;
    GLubyte b;
    GLubyte a;
    
    static const Color4B WHITE;
    static const Color4B YELLOW;
    static const Color4B BLUE;
    static const Color4B GREEN;
    static const Color4B RED;
    static const Color4B MAGENTA;
    static const Color4B BLACK;
    static const Color4B ORANGE;
    static const Color4B GRAY;
};


/** RGBA color composed of 4 floats
 @since v3.0
 */
struct Color4F
{
    Color4F();
    Color4F(float _r, float _g, float _b, float _a);
    explicit Color4F(const Color3B& color);
    explicit Color4F(const Color4B& color);
    
    bool operator==(const Color4F& right) const;
    bool operator==(const Color3B& right) const;
    bool operator==(const Color4B& right) const;
    bool operator!=(const Color4F& right) const;
    bool operator!=(const Color3B& right) const;
    bool operator!=(const Color4B& right) const;
    
    bool equals(const Color4F &other) const
    {
        return (*this == other);
    }
    
    GLfloat r;
    GLfloat g;
    GLfloat b;
    GLfloat a;
    
    static const Color4F WHITE;
    static const Color4F YELLOW;
    static const Color4F BLUE;
    static const Color4F GREEN;
    static const Color4F RED;
    static const Color4F MAGENTA;
    static const Color4F BLACK;
    static const Color4F ORANGE;
    static const Color4F GRAY;
};

struct Vertex3F {
    Vertex3F() : x(0), y(0), z(0) {}

    float x;
    float y;
    float z;
};

struct Tex2F {
    Tex2F() : u(0), v(0) {}

    float u;
    float v;
};

struct V3F_C4F_T2F {
    Vertex3F vertices;
    Color4F colors;
    Tex2F texCoords;
};

struct V3F_C4F_T2F_Quad {
    //! top left
    V3F_C4F_T2F    tl;
    //! bottom left
    V3F_C4F_T2F    bl;
    //! top right
    V3F_C4F_T2F    tr;
    //! bottom right
    V3F_C4F_T2F    br;
};

#define V3F_C4F_T2F_SIZE sizeof(NS_GLX::V3F_C4F_T2F)

#define Vertex3F_OFFSET offsetof(NS_GLX::V3F_C4F_T2F, vertices)
#define Color4F_OFFSET offsetof(NS_GLX::V3F_C4F_T2F, colors)
#define Tex2F_OFFSET offsetof(NS_GLX::V3F_C4F_T2F, texCoords)

// FIXME:: If any of these enums are edited and/or reordered, update Texture2D.m
//! Vertical text alignment type
enum class TextVAlignment
{
    TOP,
    CENTER,
    BOTTOM
};

// FIXME:: If any of these enums are edited and/or reordered, update Texture2D.m
//! Horizontal text alignment type
enum class TextHAlignment
{
    LEFT,
    CENTER,
    RIGHT
};

/**
 types used for defining fonts properties (i.e. font name, size, stroke or shadow)
 */


// shadow attributes
struct FontShadow
{
public:
    
    // shadow is not enabled by default
    FontShadow()
    : _shadowEnabled(false)
    , _shadowBlur(0)
    , _shadowOpacity(0)
    {}
    
    // true if shadow enabled
    bool   _shadowEnabled;
    // shadow x and y offset
    Size   _shadowOffset;
    // shadow blurrines
    float  _shadowBlur;
    // shadow opacity
    float  _shadowOpacity;
};

// stroke attributes
struct FontStroke
{
public:
    
    // stroke is disabled by default
    FontStroke()
    : _strokeEnabled(false)
    , _strokeColor(Color3B::BLACK)
    , _strokeAlpha(255)
    , _strokeSize(0)
    {}
    
    // true if stroke enabled
    bool      _strokeEnabled;
    // stroke color
    Color3B   _strokeColor;
    // stroke alpha
    GLubyte   _strokeAlpha;
    // stroke size
    float     _strokeSize;
    
};

// font attributes
struct FontDefinition
{
public:
    /**
     * @js NA
     * @lua NA
     */
    FontDefinition()
    : _fontSize(0)
    , _alignment(TextHAlignment::CENTER)
    , _vertAlignment(TextVAlignment::TOP)
    , _dimensions(Size::ZERO)
    , _fontFillColor(Color3B::WHITE)
    , _fontAlpha(255)
    {}
    
    // font name
    std::string           _fontName;
    // font size
    int                   _fontSize;
    // horizontal alignment
    TextHAlignment        _alignment;
    // vertical alignment
    TextVAlignment _vertAlignment;
    // renering box
    Size                  _dimensions;
    // font color
    Color3B               _fontFillColor;
    //font alpha
    GLubyte               _fontAlpha;
    // font shadow
    FontShadow            _shadow;
    // font stroke
    FontStroke            _stroke;
    
};

NS_GLX_END

#endif //!CS_TYPES_H
