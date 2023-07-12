#pragma once
#include <algorithm>

template<class T>
struct TVec2 {
    T x, y;
    TVec2(T x, T y) : x(x), y(y) {}
    TVec2(T x = 0) : TVec2(x, x) {}
    template<class U> TVec2(TVec2<U> const& v) : x(v.x), y(v.y) {}
    bool operator==(TVec2 const& v) const { return x == v.x && y == v.y; }
    bool operator!=(TVec2 const& v) const { return !(*this == v); }
    TVec2 operator+(TVec2 const& v) const { return TVec2(x + v.x, y + v.y); }
    TVec2 operator-(TVec2 const& v) const { return TVec2(x - v.x, y - v.y); }
    TVec2 operator*(T f) const { return TVec2(x * f, y * f); }
};

template<class T>
struct TVec4 {
    T x, y, z, w;
    TVec4(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}
    TVec4(T x = 0) : TVec4(x, x, x, x) {}
    template<class U> TVec4(TVec4<U> const& v) : x(v.x), y(v.y), z(v.z), w(v.w) {}
    bool operator==(TVec4 const& v) const { return x == v.x && y == v.y && z == v.z && w == v.w; }
    bool operator!=(TVec4 const& v) const { return !(*this == v); }
};

template<class T>
TVec2<T> max(TVec2<T> const& a, TVec2<T> const& b) {
    return {std::max(a.x, b.x), std::max(a.y, b.y)};
}

template<class T>
T clamp(T const& v, T const& minv, T const& maxv) {
    return std::max(minv, std::min(maxv, v));
}

template<class T>
TVec2<T> clamp(TVec2<T> const& v, TVec2<T> const& minv, TVec2<T> const& maxv) {
    return { clamp(v.x, minv.x, maxv.x), clamp(v.y, minv.y, maxv.y) };
}


using Vec2f = TVec2<float>;
using Vec4f = TVec4<float>;

using Vec2i = TVec2<int>;
using Vec4i = TVec4<int>;
