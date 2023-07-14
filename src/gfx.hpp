#pragma once
#include "vec.hpp"
#include <vector>


namespace gfx {


using Vec = TVec2<int16_t>;
using Col = TVec4<uint8_t>;


struct Vertex {
    Vec pos;
    Vec uv;
    Col col;
};


class DrawContext {
public:

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

    void rect(Vec pos, Vec size, Vec uv, Col col) {
        uint16_t i0 = add_vertex({ pos, uv, col });
        uint16_t i1 = add_vertex({ pos + Vec(size.x, 0), uv + Vec(size.x, 0), col });
        uint16_t i2 = add_vertex({ pos + size, uv + size, col });
        uint16_t i3 = add_vertex({ pos + Vec(0, size.y), uv + Vec(0, size.y), col });
        m_indices.insert(m_indices.end(), { i0, i1, i2, i0, i2, i3 });
    }

    std::vector<Vertex> const& vertices() const { return m_vertices; }
    std::vector<uint16_t> const& indices() const { return m_indices; }

private:
    std::vector<Vertex>   m_vertices;
    std::vector<uint16_t> m_indices;
};


class Texture {
friend void draw(DrawContext const& dc, Texture const& tex);
public:
    int width() const { return m_width; }
    int height() const { return m_height; }
    void free();
protected:
    Texture() {}
    void init(int w, int h, uint8_t const* pixels);
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_gl_texture = 0;
};


class Canvas : public Texture {
friend void set_canvas(Canvas const& canvas);
public:
    void init(int w, int h);
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

void resize(int width, int height);
int  screen_width();
int  screen_height();

void set_blend(bool enabled);
void set_canvas();
void set_canvas(Canvas const& canvas);

void clear(float r, float g, float b);
void draw(DrawContext const& dc, Texture const& tex);

} // namespace gfx
