/*
Project: Urban Sprawl
File: urban_math.h
Author: Brock Salmon
Notice: (C) Copyright 2018 by Brock Salmon. All Rights Reserved.
*/

#ifndef URBAN_MATH_H

// VECTOR NOTES
// NOTE(bSalmon): Less than and greater than operators work off if EVERY coord
// in the Vector is less/greater than the other coords

// 2D VECTOR
template <class type>
struct v2
{
    union
    {
        type elem[2];
        struct
        {
            type x;
            type y;
        };
    };
    
    v2<type>& operator+=(v2<type> a);
    v2<type>& operator-=(v2<type> a);
    v2<type>& operator*=(type val);
    v2<type>& operator-();
};

template <class type>
v2<type>& v2<type>::operator+=(v2<type> a)
{
    this->x += a.x;
    this->y += a.y;
    
    return *this;
}

template <class type>
v2<type>& v2<type>::operator-=(v2<type> a)
{
    this->x -= a.x;
    this->y -= a.y;
    
    return *this;
}

template <class type>
v2<type>& v2<type>::operator*=(type val)
{
    this->x *= val;
    this->y *= val;
    
    return *this;
}

template <class type>
v2<type>& v2<type>::operator-()
{
    this->x = -this->x;
    this->y = -this->y;
    
    return *this;
}

template <class type, class type_two>
v2<type> operator+(v2<type> a, v2<type_two> b)
{
    v2<type> result = {};
    
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    
    return result;
}

template <class type, class type_two>
v2<type> operator+(v2<type> vec, type_two val)
{
    v2<type> result = {};
    
    result.x = vec.x + val;
    result.y = vec.y + val;
    
    return result;
}

template <class type, class type_two>
v2<type> operator-(v2<type> a, v2<type_two> b)
{
    v2<type> result = {};
    
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    
    return result;
}

template <class type, class type_two>
v2<type> operator-(v2<type> vec, type_two val)
{
    v2<type> result = {};
    
    result.x = vec.x - val;
    result.y = vec.y - val;
    
    return result;
}

template <class type, class type_two>
type operator*(v2<type> a, v2<type_two> b)
{
    type result = 0;
    
    result = (a.x * b.x) + (a.y * b.y);
    
    return result;
}

template <class type, class type_two>
v2<type> operator*(v2<type> a, type_two b)
{
    v2<type> result = {};
    
    result.x = (a.x * b);
    result.y = (a.y * b);
    
    return result;
}

template <class type, class type_two>
b32 operator<(v2<type> a, v2<type_two> b)
{
    b32 result = false;
    
    result = ((a.x < b.x) &&
              (a.y < b.y));
    
    return result;
}

template <class type, class type_two>
b32 operator<=(v2<type> a, v2<type_two> b)
{
    b32 result = false;
    
    result = ((a.x <= b.x) &&
              (a.y <= b.y));
    
    return result;
}

template <class type, class type_two>
b32 operator>(v2<type> a, v2<type_two> b)
{
    b32 result = false;
    
    result = ((a.x > b.x) &&
              (a.y > b.y));
    
    return result;
}

template <class type, class type_two>
b32 operator>=(v2<type> a, v2<type_two> b)
{
    b32 result = false;
    
    result = ((a.x >= b.x) &&
              (a.y >= b.y));
    
    return result;
}

template <class type, class type_two>
b32 operator==(v2<type> a, v2<type_two> b)
{
    b32 result = false;
    
    result = ((a.x == b.x) &&
              (a.y == b.y));
    
    return result;
}

// 3D VECTOR
template <class type>
struct v3
{
    union
    {
        type elem[3];
        struct
        {
            type x;
            type y;
            type z;
        };
    };
    
    v3<type>& operator+=(v3<type> a);
    v3<type>& operator-=(v3<type> a);
    v3<type>& operator*=(type val);
    v3<type>& operator-();
};

template <class type>
v3<type>& v3<type>::operator+=(v3<type> a)
{
    this->x += a.x;
    this->y += a.y;
    this->z += a.z;
    
    return *this;
}

template <class type>
v3<type>& v3<type>::operator-=(v3<type> a)
{
    this->x -= a.x;
    this->y -= a.y;
    this->z -= a.z;
    
    return *this;
}

template <class type>
v3<type>& v3<type>::operator*=(type val)
{
    this->x *= val;
    this->y *= val;
    this->z *= val;
    
    return *this;
}

template <class type>
v3<type>& v3<type>::operator-()
{
    this->x = -this->x;
    this->y = -this->y;
    this->z = -this->z;
    
    return *this;
}

template <class type, class type_two>
v3<type> operator+(v3<type> a, v3<type_two> b)
{
    v3<type> result = {};
    
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    
    return result;
}

template <class type, class type_two>
v3<type> operator+(v3<type> vec, type_two val)
{
    v3<type> result = {};
    
    result.x = vec.x + val;
    result.y = vec.y + val;
    result.z = vec.z + val;
    
    return result;
}

template <class type, class type_two>
v3<type> operator-(v3<type> a, v3<type_two> b)
{
    v3<type> result = {};
    
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    
    return result;
}

template <class type, class type_two>
v3<type> operator-(v3<type> vec, type_two val)
{
    v3<type> result = {};
    
    result.x = vec.x - val;
    result.y = vec.y - val;
    result.z = vec.z - val;
    
    return result;
}

template <class type, class type_two>
type operator*(v3<type> a, v3<type_two> b)
{
    type result = 0;
    
    result = (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
    
    return result;
}

template <class type, class type_two>
v3<type> operator*(v3<type> a, type_two b)
{
    v3<type> result = {};
    
    result.x = (a.x * b);
    result.y = (a.y * b);
    result.z = (a.z * b);
    
    return result;
}

template <class type, class type_two>
b32 operator<(v3<type> a, v3<type_two> b)
{
    b32 result = false;
    
    result = ((a.x < b.x) &&
              (a.y < b.y) &&
              (a.z < b.z));
    
    return result;
}

template <class type, class type_two>
b32 operator<=(v3<type> a, v3<type_two> b)
{
    b32 result = false;
    
    result = ((a.x <= b.x) &&
              (a.y <= b.y) &&
              (a.z <= b.z));
    
    return result;
}

template <class type, class type_two>
b32 operator>(v3<type> a, v3<type_two> b)
{
    b32 result = false;
    
    result = ((a.x > b.x) &&
              (a.y > b.y) &&
              (a.z > b.z));
    
    return result;
}

template <class type, class type_two>
b32 operator>=(v3<type> a, v3<type_two> b)
{
    b32 result = false;
    
    result = ((a.x >= b.x) &&
              (a.y >= b.y) &&
              (a.z >= b.z));
    
    return result;
}

template <class type, class type_two>
b32 operator==(v3<type> a, v3<type_two> b)
{
    b32 result = false;
    
    result = ((a.x == b.x) &&
              (a.y == b.y) &&
              (a.z == b.z));
    
    return result;
}

template <class type>
type Sq(type a)
{
    type result = 0;
    
    result = a * a;
    
    return result;
}

template <class type, class type_two>
type Inner(v2<type> a, v2<type_two> b)
{
    type result = 0;
    
    result = (a.x * b.x) + (a.y * b.y);
    
    return result;
}

template <class type, class type_two>
type Inner(v3<type> a, v3<type_two> b)
{
    type result = 0;
    
    result = (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
    
    return result;
}

#define URBAN_MATH_H
#endif // URBAN_MATH_H
