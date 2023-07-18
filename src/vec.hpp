#pragma once
#include <algorithm>
#include <cstdint>

template<class T>
struct TVec2 {
    T x, y;
    constexpr TVec2(T x, T y) : x(x), y(y) {}
    constexpr TVec2(T x = 0) : TVec2(x, x) {}
    TVec2 xo() const { return {x, 0}; }
    TVec2 oy() const { return {0, y}; }
    template<class U> TVec2(TVec2<U> v) : x(v.x), y(v.y) {}
    bool operator==(TVec2 v) const { return x == v.x && y == v.y; }
    bool operator!=(TVec2 v) const { return !(*this == v); }
    TVec2 operator+(TVec2 v) const { return TVec2(x + v.x, y + v.y); }
    TVec2 operator-(TVec2 v) const { return TVec2(x - v.x, y - v.y); }
    template<class U> TVec2 operator*(U v) const { return TVec2(x * v, y * v); }
    template<class U> TVec2 operator/(U v) const { return TVec2(x / v, y / v); }
};

template<class T>
struct TVec4 {
    T x, y, z, w;
    constexpr TVec4(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}
    constexpr TVec4(T x = 0) : TVec4(x, x, x, x) {}
    template<class U> TVec4(TVec4<U> v) : x(v.x), y(v.y), z(v.z), w(v.w) {}
    bool operator==(TVec4 v) const { return x == v.x && y == v.y && z == v.z && w == v.w; }
    bool operator!=(TVec4 v) const { return !(*this == v); }
};

template<class T>
TVec2<T> max(TVec2<T> a, TVec2<T> b) {
    return {std::max(a.x, b.x), std::max(a.y, b.y)};
}

template<class T>
T clamp(T v, T minv, T maxv) {
    return std::max(minv, std::min(maxv, v));
}

template<class T>
TVec2<T> clamp(TVec2<T> v, TVec2<T> minv, TVec2<T> maxv) {
    return { clamp(v.x, minv.x, maxv.x), clamp(v.y, minv.y, maxv.y) };
}


using ivec2   = TVec2<int32_t>;
using i16vec2 = TVec2<int16_t>;
using u8vec4  = TVec4<uint8_t>;
