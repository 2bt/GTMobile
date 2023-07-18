#pragma once
#include "vec.hpp"
#include <vector>


namespace gfx {



struct Vertex {
    i16vec2 pos;
    i16vec2 uv;
    u8vec4  col;
};


class DrawContext {
public:

    void color(u8vec4 color) { m_color = color; }

    uint16_t add_vertex(Vertex v) {
        uint16_t index = m_vertices.size();
        m_vertices.emplace_back(v);
        return index;
    }

    void add_index(uint16_t index) {
        m_indices.push_back(index);
    }

    void clear() {
        m_vertices.clear();
        m_indices.clear();
    }

    void rect(ivec2 pos, ivec2 size, ivec2 uv) {
        uint16_t i0 = add_vertex({ pos, uv, m_color });
        uint16_t i1 = add_vertex({ pos + ivec2(size.x, 0), uv + ivec2(size.x, 0), m_color });
        uint16_t i2 = add_vertex({ pos + size, uv + size, m_color });
        uint16_t i3 = add_vertex({ pos + ivec2(0, size.y), uv + ivec2(0, size.y), m_color });
        m_indices.insert(m_indices.end(), { i0, i1, i2, i0, i2, i3 });
    }

    std::vector<Vertex> const& vertices() const { return m_vertices; }
    std::vector<uint16_t> const& indices() const { return m_indices; }

protected:
    u8vec4                m_color = {255};
    std::vector<Vertex>   m_vertices;
    std::vector<uint16_t> m_indices;
};


class Texture {
friend void draw(DrawContext const& dc, Texture const& tex);
public:
    enum FilterMode { NEAREST, LINEAR };
    ivec2 size() const { return m_size; }
    void free();
    void set_filter(FilterMode filter);
protected:
    Texture() {}
    void init(ivec2 size, uint8_t const* pixels);
    ivec2    m_size;
    uint32_t m_gl_texture = 0;
};


class Canvas : public Texture {
friend void set_canvas(Canvas const& canvas);
public:
    void init(ivec2 size);
    void free();
protected:
    uint32_t m_gl_framebuffer;
};


class Image : public Texture {
public:
    bool init(char const* name);
};


bool init();
void free();

void set_screen_size(ivec2 size);
ivec2 screen_size();

void set_blend(bool enabled);
void set_canvas();
void set_canvas(Canvas const& canvas);

void clear(float r, float g, float b);
void draw(DrawContext const& dc, Texture const& tex);

} // namespace gfx
