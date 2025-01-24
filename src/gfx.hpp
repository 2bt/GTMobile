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
    void filter(FilterMode filter);
    void get_pixel_data(std::vector<uint8_t>& data); // for screenshots
protected:
    Texture() {}
    void init(ivec2 size, uint8_t const* pixels);
    ivec2    m_size;
    uint32_t m_gl_texture = 0;
};


class Canvas : public Texture {
friend void canvas(Canvas const& canvas);
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

void  screen_size(ivec2 size);
ivec2 screen_size();

void blend(bool enabled);
void reset_canvas();
void canvas(Canvas const& canvas);

void clear(float r, float g, float b);
void draw(Mesh const& mesh, Texture const& tex);

} // namespace gfx
