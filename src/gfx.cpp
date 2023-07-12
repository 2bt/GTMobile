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

#include "gfx.hpp"
#include "log.hpp"







namespace gfx {
namespace {
    GLuint g_program;
    int g_screen_scale_location;

    GLuint g_vao;
    GLuint g_vbo;
    GLuint g_ebo;


} // namspace



bool init() {

    const char* VERTEX_SHADER_SOURCE = R"(#version 300 es
        layout (location = 0) in vec2 a_pos;
        layout (location = 1) in vec4 a_col;
        uniform vec2 screen_scale;
        out vec4 col;
        void main() {
            gl_Position = vec4(a_pos * screen_scale - vec2(1.0), 0.0, 1.0);
            col = a_col;
        }
    )";
    const char* FRAGMENT_SHADER_SOURCE = R"(#version 300 es
        precision mediump float;
        in vec4 col;
        out vec4 FragColor;
        void main() {
            FragColor = col + vec4(1.0, 0.0, 0.0, 1.0);
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


    g_screen_scale_location = glGetUniformLocation(g_program, "screen_scale");

    glGenVertexArrays(1, &g_vao);
    glBindVertexArray(g_vao);
    glGenBuffers(1, &g_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
    glGenBuffers(1, &g_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ebo);

    glVertexAttribPointer(0, 2, GL_SHORT, GL_FALSE, sizeof(Vertex), (void*) 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void*) 4);
    glEnableVertexAttribArray(1);

    return true;
}

void free() {
    glDeleteVertexArrays(1, &g_vao);
    glDeleteBuffers(1, &g_vbo);
    glDeleteBuffers(1, &g_ebo);
    glDeleteProgram(g_program);
}


void set_viewport(int width, int height) {
    glViewport(0, 0, width, height);

    glUseProgram(g_program);
    glUniform2f(g_screen_scale_location, 2.0f / width, 2.0f / height);
}

void clear() {
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void draw(DrawContext const& dc) {

    glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(Vertex) * dc.vertices().size(),
                 dc.vertices().data(),
                 GL_STREAM_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 sizeof(uint16_t) * dc.indices().size(),
                 dc.indices().data(),
                 GL_STREAM_DRAW);


//    g_shader->set_uniform("texture_scale", Vec2f(1.0f / texture->size().x, 1.0f / texture->size().y));

    glUseProgram(g_program);
    glBindVertexArray(g_vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
}


} // namespace gfx
