#pragma once

#include "Common.h"
#include <sstream>

typedef int PositionType;

namespace BOSS
{
class Position
{

    PositionType    _x; 
    PositionType    _y;

public:
 
    Position()
        : _x(0)
        , _y(0)
    {
    }

    Position(const PositionType & x, const PositionType & y)
        : _x(x)
        , _y(y)
    {
    }


    bool operator < (const Position & rhs) const
    {
        return (x() < rhs.x()) || ((x() == rhs.x()) && y() < rhs.y());
    }

    bool operator == (const Position & rhs) const
    {
        return x() == rhs.x() && y() == rhs.y();
    }

    Position operator + (const Position & rhs) const
    {
        return Position(x() + rhs.x(), y() + rhs.y());
    }

    Position operator - (const Position & rhs) const
    {
        return Position(x() - rhs.x(), y() - rhs.y());
    }

    Position operator / (const PositionType & d) const
    {
        return Position(_x / d, _y / d);
    }

    Position operator * (const PositionType & d) const
    {
        return Position(_x * d, _y * d);
    }

    Position scale(const float & f) const
    {
        return Position((PositionType)(f * x()), (PositionType)(f * y()));
    }

    void scalePosition(const float & f)
    {
        _x = (PositionType)(f * _x);
        _y = (PositionType)(f * _y);
    }

    void add(const Position & rhs)
    {
        _x += rhs.x();
        _y += rhs.y();
    }

    void subtract(const Position & rhs)
    {
        _x -= rhs.x();
        _y -= rhs.y();
    }
 
    void moveTo(const Position & pos)
    {
        _x = pos.x();
        _y = pos.y();
    }

    void add(const PositionType & x, const PositionType & y)
    {
        _x += x;
        _y += y;
    }

    void moveTo(const PositionType & x, const PositionType & y)
    {
        _x = x;
        _y = y;
    }

    PositionType x() const
    {
        return _x;
    }

    PositionType y() const
    {
        return _y;
    }

    Position flipX() const
    {
        return Position(-_x,_y);
    }

    Position flipY() const
    {
        return Position(_y,_x);
    }

    float Q_rsqrt( float number ) const
    {
        long i;
        float x2, y;
        const float threehalfs = 1.5F;
 
        x2 = number * 0.5F;
        y  = number;
        i  = * ( long * ) &y;                       // evil floating point bit level hacking
        i  = 0x5f3759df - ( i >> 1 );               
        y  = * ( float * ) &i;
        y  = y * ( threehalfs - ( x2 * y * y ) );   // 1st iteration
//      y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed
 
        return y;
    }

    Position flip() const
    {
        return Position(-_x, -_y);
    }

    PositionType getDistance(const Position & p) const 
    {
        PositionType dX = x() - p.x();
        PositionType dY = y() - p.y();

        if (dX == 0)
        {
            return abs(dY);
        }
        else if (dY == 0)
        {
            return abs(dX);
        }
        else
        {
            return (PositionType)sqrt((float)(dX*dX - dY*dY));
        }
     }

    PositionType getDistanceSq(const Position & p) const 
    {
        return (x()-p.x())*(x()-p.x()) + (y()-p.y())*(y()-p.y());
    }

    void print() const
    {
        printf("Position = (%d, %d)\n", _x, _y);
    }

    std::string getString() const
    {
        std::stringstream ss;
        ss << "(" << x() << ", " << y() << ")";
        return ss.str();
    }

};
}
