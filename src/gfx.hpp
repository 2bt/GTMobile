#pragma once
#include "vec.hpp"
#include <vector>


namespace gfx {



struct Vertex {
    i16vec2 pos;
    i16vec2 uv;
    u8vec4  col;
};


struct Mesh {
    std::vector<Vertex>   vertices;
    std::vector<uint16_t> indices;
};


class Texture {
friend void draw(Mesh const& mesh, Texture const& tex);
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
void draw(Mesh const& mesh, Texture const& tex);

} // namespace gfx
