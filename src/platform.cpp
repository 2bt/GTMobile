#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <fstream>
#include "platform.hpp"
#include "app.hpp"
#include "log.hpp"


namespace {

SDL_AudioDeviceID g_audio_device = 0;

} // namespace

namespace platform {

bool start_audio() {
    LOGD("start_audio");
    if (g_audio_device != 0) return true;
//    int n = SDL_GetNumAudioDevices(0);
//    for (int i = 0; i < n; ++i) {
//        LOGI("start_audio: device %d: %s", i, SDL_GetAudioDeviceName(i, 0));
//    }
    SDL_AudioSpec spec = {
        app::MIXRATE, AUDIO_S16, 1, 0, 1024 * 3, 0, 0, [](void*, Uint8* stream, int len) {
            app::audio_callback((short*) stream, len / 2);
        },
    };
    g_audio_device = SDL_OpenAudioDevice(nullptr, 0, &spec, nullptr, 0);
    SDL_PauseAudioDevice(g_audio_device, 0);
    return true;
}

void stop_audio() {
    LOGD("stop_audio");
    SDL_CloseAudioDevice(g_audio_device);
    g_audio_device = 0;
}


bool load_asset(std::string const& name, std::vector<uint8_t>& buf) {
    std::ifstream f("assets/" + name, std::ios::in | std::ios::binary);
    if (!f.is_open()) {
        LOGE("load_asset: could not open %s", name.c_str());
        return false;
    }
    buf = std::vector<uint8_t>((std::istreambuf_iterator<char>(f)),
                                std::istreambuf_iterator<char>());
    return true;
}

} // namespace platform



int main(int argc, char** argv) {

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);

    SDL_Window* window = SDL_CreateWindow(
            "GTMobile",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            app::WIDTH, app::MIN_HEIGHT,
            SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        LOGE("main: SDL_GL_CreateContext() failed");
        SDL_GL_DeleteContext(gl_context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_GL_SetSwapInterval(1);
    glewExperimental = true;
    glewInit();

//    platform::start_audio();
    app::init();
    app::resize(app::WIDTH, app::MIN_HEIGHT);

    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_QUIT:
                running = false;
                break;

            case SDL_KEYDOWN:
                switch (e.key.keysym.scancode) {
                case SDL_SCANCODE_ESCAPE:
                    running = false;
                    break;
                case SDL_SCANCODE_RETURN:
                    app::key(app::KEYCODE_ENTER, 0);
                    break;
                case SDL_SCANCODE_BACKSPACE:
                    app::key(app::KEYCODE_DEL, 0);
                    break;
                default: break;
                }
                break;
            case SDL_TEXTINPUT:
                app::key(0, e.text.text[0]);
                break;

            case SDL_WINDOWEVENT:
                if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    app::resize(e.window.data1, e.window.data2);
                }
                break;

            case SDL_MOUSEBUTTONDOWN:
                if (e.button.button != SDL_BUTTON_LEFT) break;
                app::touch(e.motion.x, e.motion.y, true);
                break;
            case SDL_MOUSEBUTTONUP:
                if (e.button.button != SDL_BUTTON_LEFT) break;
                app::touch(e.motion.x, e.motion.y, false);
                break;
            case SDL_MOUSEMOTION:
                if (!(e.motion.state & SDL_BUTTON_LMASK)) break;
                app::touch(e.motion.x, e.motion.y, true);
                break;

            default: break;
            }
        }

        app::draw();
        SDL_GL_SwapWindow(window);
    }

    app::free();
    platform::stop_audio();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
