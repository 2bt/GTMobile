#ifdef ANDROID
#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#define glBindVertexArray    glBindVertexArrayOES
#define glGenVertexArrays    glGenVertexArraysOES
#define glDeleteVertexArrays glDeleteVertexArraysOES
#else
#include <GL/glew.h>
#endif

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_FAILURE_STRINGS
#define STBI_ONLY_PNG
#include "stb_image.h"

#include "log.hpp"
#include "platform.hpp"
#include "gfx.hpp"

#include <cassert>



namespace gfx {



void Texture::free() {
    if (m_gl_texture > 0) {
        glDeleteTextures(1, &m_gl_texture);
        m_gl_texture = 0;
    }
}
void Texture::init(ivec2 size, uint8_t const* pixels) {
    free();
    m_size = size;
    glGenTextures(1, &m_gl_texture);
    //glBindTexture(GL_TEXTURE_2D, m_gl_texture);
    set_filter(NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
}
void Texture::set_filter(FilterMode filter) {
    GLint f = filter == LINEAR ? GL_LINEAR : GL_NEAREST;
    glBindTexture(GL_TEXTURE_2D, m_gl_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, f);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, f);
}

void Canvas::init(ivec2 size) {
    free();
    Texture::init(size, nullptr);
    glGenFramebuffers(1, &m_gl_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_gl_framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_gl_texture, 0);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        LOGE("Canvas::init: bad frame buffer status");
    }
}
void Canvas::free() {
    Texture::free();
    if (m_gl_framebuffer > 0) {
        glDeleteFramebuffers(1, &m_gl_framebuffer);
        m_gl_framebuffer = 0;
    }
}


bool Image::init(char const* name) {
    free();
    std::vector<uint8_t> buf;
    if (!platform::load_asset(name, buf)) return false;
    int w, h, c;
    uint8_t* p = stbi_load_from_memory(buf.data(), buf.size(), &w, &h, &c, 0);
    assert(c == 4);
    Texture::init({w, h}, p);
    stbi_image_free(p);
    return true;
}

namespace {

    ivec2  g_screen_size;

    GLuint g_program;
    GLint  g_pos_scale_loc;
    GLint  g_uv_scale_loc;
    GLint  g_tex_loc;

    GLuint g_vao;
    GLuint g_vbo;
    GLuint g_ebo;


} // namspace


void set_canvas() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(g_program);
    glUniform2f(g_pos_scale_loc, 2.0f / g_screen_size.x, 2.0f / g_screen_size.y);
    glViewport(0, 0, g_screen_size.x, g_screen_size.y);
}
void set_canvas(Canvas const& canvas) {
    glBindFramebuffer(GL_FRAMEBUFFER, canvas.m_gl_framebuffer);
    glUseProgram(g_program);
    glUniform2f(g_pos_scale_loc, 2.0f / canvas.size().x, 2.0f / canvas.size().y);
    glViewport(0, 0, canvas.size().x, canvas.size().y);
}



bool init() {

    const char* VERTEX_SHADER_SOURCE = R"(#version 300 es
        layout (location = 0) in vec2 i_pos;
        layout (location = 1) in vec2 i_uv;
        layout (location = 2) in vec4 i_col;
        uniform vec2 u_pos_scale;
        uniform vec2 u_uv_scale;
        out vec2 v_uv;
        out vec4 v_col;
        void main() {
            gl_Position = vec4(i_pos * u_pos_scale - vec2(1.0), 0.0, 1.0);
            v_uv  = i_uv * u_uv_scale;
            v_col = i_col;
        }
    )";
    const char* FRAGMENT_SHADER_SOURCE = R"(#version 300 es
        precision mediump float;
        in vec2 v_uv;
        in vec4 v_col;
        out vec4 FragColor;
        uniform sampler2D u_tex;
        void main() {
            FragColor = v_col * texture(u_tex, v_uv);
        }
    )";

    GLint ok;
    GLchar log[1024];

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &VERTEX_SHADER_SOURCE, nullptr);
    glCompileShader(vertex_shader);
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        glGetShaderInfoLog(vertex_shader, sizeof(log), nullptr, log);
        LOGE("failed to compile vertex shader:\n%s", log);
        return false;
    }

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &FRAGMENT_SHADER_SOURCE, nullptr);
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        glGetShaderInfoLog(fragment_shader, sizeof(log), nullptr, log);
        LOGE("failed to compile fragment shader:\n%s", log);
        return false;
    }

    g_program = glCreateProgram();
    glAttachShader(g_program, vertex_shader);
    glAttachShader(g_program, fragment_shader);
    glLinkProgram(g_program);
    glGetProgramiv(g_program, GL_LINK_STATUS, &ok);
    if (!ok) {
        glGetShaderInfoLog(g_program, sizeof(log), nullptr, log);
        LOGE("failed to link shader program:\n%s", log);
        return false;
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);


    g_pos_scale_loc = glGetUniformLocation(g_program, "u_pos_scale");
    g_uv_scale_loc  = glGetUniformLocation(g_program, "u_uv_scale");
    g_tex_loc       = glGetUniformLocation(g_program, "u_tex");

    glGenVertexArrays(1, &g_vao);
    glBindVertexArray(g_vao);
    glGenBuffers(1, &g_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
    glGenBuffers(1, &g_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ebo);

    glVertexAttribPointer(0, 2, GL_SHORT, GL_FALSE, sizeof(Vertex), (void*) 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_SHORT, GL_FALSE, sizeof(Vertex), (void*) 4);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void*) 8);
    glEnableVertexAttribArray(2);

    return true;
}

void free() {
    glDeleteVertexArrays(1, &g_vao);
    glDeleteBuffers(1, &g_vbo);
    glDeleteBuffers(1, &g_ebo);
    glDeleteProgram(g_program);
}


void set_screen_size(ivec2 size) { g_screen_size = size; }
ivec2 screen_size() { return g_screen_size; }


void clear(float r, float g, float b) {
    glClearColor(r, g, b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void set_blend(bool enabled) {
    if (enabled) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else {
        glDisable(GL_BLEND);
    }
}

void draw(Mesh const& mesh, Texture const& tex) {

    glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(Vertex) * mesh.vertices.size(),
                 mesh.vertices.data(),
                 GL_STREAM_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 sizeof(uint16_t) * mesh.indices.size(),
                 mesh.indices.data(),
                 GL_STREAM_DRAW);

    glUniform2f(g_uv_scale_loc, 1.0f / tex.m_size.x, 1.0f / tex.m_size.y);
    glBindTexture(GL_TEXTURE_2D, tex.m_gl_texture);
    glUniform1i(g_tex_loc, 0);

    glBindVertexArray(g_vao);
    glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_SHORT, 0);
}


} // namespace gfx
