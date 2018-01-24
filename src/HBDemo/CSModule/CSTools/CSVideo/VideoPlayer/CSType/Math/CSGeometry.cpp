//
//  Geometry.cpp
//  mtmv
//
//  Created by Javan on 15/8/17.
//  Copyright (c) 2015年 meitu. All rights reserved.
//

#include "CSGeometry.h"

#include <cassert>
#include <cfloat>

NS_GLX_BEGIN

Size::Size(void) : width(0), height(0)
{
}

Size::Size(float w, float h) : width(w), height(h)
{
}

Size::Size(const Size& other) : width(other.width), height(other.height)
{
}

Size::Size(const Vec2& point) : width(point.x), height(point.y)
{
}

Size& Size::operator= (const Size& other)
{
    setSize(other.width, other.height);
    return *this;
}

Size& Size::operator= (const Vec2& point)
{
    setSize(point.x, point.y);
    return *this;
}

Size Size::operator+(const Size& right) const
{
    return Size(this->width + right.width, this->height + right.height);
}

Size Size::operator-(const Size& right) const
{
    return Size(this->width - right.width, this->height - right.height);
}

Size Size::operator*(float a) const
{
    return Size(this->width * a, this->height * a);
}

Size Size::operator/(float a) const
{
    assert(a!=0);
    return Size(this->width / a, this->height / a);
}

void Size::setSize(float w, float h)
{
    this->width = w;
    this->height = h;
}

bool Size::equals(const Size& target) const
{
    return (fabs(this->width  - target.width)  < FLT_EPSILON)
    && (fabs(this->height - target.height) < FLT_EPSILON);
}

const Size Size::ZERO = Size(0, 0);

// implementation of Rect

Rect::Rect(void)
{
    setRect(0.0f, 0.0f, 0.0f, 0.0f);
}

Rect::Rect(float x, float y, float width, float height)
{
    setRect(x, y, width, height);
}

Rect::Rect(const Rect& other)
{
    setRect(other.origin.x, other.origin.y, other.size.width, other.size.height);
}

Rect& Rect::operator= (const Rect& other)
{
    setRect(other.origin.x, other.origin.y, other.size.width, other.size.height);
    return *this;
}

void Rect::setRect(float x, float y, float width, float height)
{
    // CGRect can support width<0 or height<0
    // CCASSERT(width >= 0.0f && height >= 0.0f, "width and height of Rect must not less than 0.");
    
    origin.x = x;
    origin.y = y;
    
    size.width = width;
    size.height = height;
}

bool Rect::equals(const Rect& rect) const
{
    return (origin.equals(rect.origin) &&
            size.equals(rect.size));
}

float Rect::getMaxX() const
{
    return origin.x + size.width;
}

float Rect::getMidX() const
{
    return origin.x + size.width / 2.0f;
}

float Rect::getMinX() const
{
    return origin.x;
}

float Rect::getMaxY() const
{
    return origin.y + size.height;
}

float Rect::getMidY() const
{
    return origin.y + size.height / 2.0f;
}

float Rect::getMinY() const
{
    return origin.y;
}

bool Rect::containsPoint(const Vec2& point) const
{
    bool bRet = false;
    
    if (point.x >= getMinX() && point.x <= getMaxX()
        && point.y >= getMinY() && point.y <= getMaxY())
    {
        bRet = true;
    }
    
    return bRet;
}

bool Rect::intersectsRect(const Rect& rect) const
{
    return !(     getMaxX() < rect.getMinX() ||
             rect.getMaxX() <      getMinX() ||
             getMaxY() < rect.getMinY() ||
             rect.getMaxY() <      getMinY());
}

void Rect::merge(const Rect& rect)
{
    float top1    = getMaxY();
    float left1   = getMinX();
    float right1  = getMaxX();
    float bottom1 = getMinY();
    
    float top2    = rect.getMaxY();
    float left2   = rect.getMinX();
    float right2  = rect.getMaxX();
    float bottom2 = rect.getMinY();
    origin.x = std::min(left1, left2);
    origin.y = std::min(bottom1, bottom2);
    size.width = std::max(right1, right2) - origin.x;
    size.height = std::max(top1, top2) - origin.y;
}

Rect Rect::unionWithRect(const Rect & rect) const
{
    float thisLeftX = origin.x;
    float thisRightX = origin.x + size.width;
    float thisTopY = origin.y + size.height;
    float thisBottomY = origin.y;
    
    if (thisRightX < thisLeftX)
    {
        std::swap(thisRightX, thisLeftX);   // This rect has negative width
    }
    
    if (thisTopY < thisBottomY)
    {
        std::swap(thisTopY, thisBottomY);   // This rect has negative height
    }
    
    float otherLeftX = rect.origin.x;
    float otherRightX = rect.origin.x + rect.size.width;
    float otherTopY = rect.origin.y + rect.size.height;
    float otherBottomY = rect.origin.y;
    
    if (otherRightX < otherLeftX)
    {
        std::swap(otherRightX, otherLeftX);   // Other rect has negative width
    }
    
    if (otherTopY < otherBottomY)
    {
        std::swap(otherTopY, otherBottomY);   // Other rect has negative height
    }
    
    float combinedLeftX = std::min(thisLeftX, otherLeftX);
    float combinedRightX = std::max(thisRightX, otherRightX);
    float combinedTopY = std::max(thisTopY, otherTopY);
    float combinedBottomY = std::min(thisBottomY, otherBottomY);
    
    return Rect(combinedLeftX, combinedBottomY, combinedRightX - combinedLeftX, combinedTopY - combinedBottomY);
}

const Rect Rect::ZERO = Rect(0, 0, 0, 0);

NS_GLX_END
