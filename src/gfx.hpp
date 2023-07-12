#pragma once
#include "vec.hpp"
#include <vector>

namespace gfx {

using Vec = TVec2<int16_t>;
using Col = TVec4<uint8_t>;

struct Vertex {
    Vec pos;
//    Vec uv;
    Col  col;
};


class DrawContext {
public:

    uint16_t add_vertex(Vertex v) {
        uint16_t index = m_vertices.size();
        m_vertices.emplace_back(v);
        return index;
    }

    void clear() {
        m_vertices.clear();
        m_indices.clear();
    }

    void rect(Vec pos, Vec size, Col col) {

        uint16_t i0 = add_vertex({ pos, col });
        uint16_t i1 = add_vertex({ pos + Vec(size.x, 0), col });
        uint16_t i2 = add_vertex({ pos + size, col });
        uint16_t i3 = add_vertex({ pos + Vec(0, size.y), col });

        m_indices.insert(m_indices.end(), { i0, i1, i2, i0, i2, i3 });
    }

//    void copy(i16vec2 pos, i16vec2 size, i16vec2 uv, i16vec2 uv_size, u8vec4 c) {
//        quad({ pos, uv, c},
//             { pos + i16vec2(0, size.y), uv + i16vec2(0, uv_size.y), c},
//             { pos + i16vec2(size.x, 0), uv + i16vec2(uv_size.x, 0), c},
//             { pos + size, uv + uv_size, c});
//    }

//    void copy(i16vec2 pos, i16vec2 size, i16vec2 uv, u8vec4 c = {255, 255, 255, 255}) {
//        copy(pos, size, uv, size, c);
//    }

    std::vector<Vertex> const& vertices() const { return m_vertices; }
    std::vector<uint16_t> const& indices() const { return m_indices; }

private:
    std::vector<Vertex>   m_vertices;
    std::vector<uint16_t> m_indices;
};


bool init();
void free();
void set_viewport(int width, int height);
void clear();
void draw(DrawContext const& dc);


} // namespace gfx
