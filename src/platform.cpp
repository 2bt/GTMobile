#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include "platform.hpp"
#include "app.hpp"
#include "gui.hpp"
#include "log.hpp"
#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#else
#define PORTMIDI
#include <portmidi.h>
#endif


namespace {


bool              g_running     = true;
SDL_Window*       g_window      = nullptr;
SDL_AudioDeviceID g_audio_device = 0;
PortMidiStream*   g_midi_stream  = nullptr;


bool start_audio() {
    LOGD("start_audio");
    if (g_audio_device != 0) return true;
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

enum {
    MAX_MIDI_EVENTS = 64,
};

bool init_midi() {
    Pm_Initialize();
    int my_id;
    PmDeviceInfo const* my_info = nullptr;
    PmDeviceInfo const* info;
    for (int i = 0; (info = Pm_GetDeviceInfo(i)); ++i) {
        if (!info->input) continue;
        my_info = info;
        my_id   = i;
    }
    if (!my_info) {
        LOGD("init_midi: found no MIDI input device");
        return false;
    }
    LOGD("init_midi: found MIDI input device %d: %s - %s", my_id, my_info->interf, my_info->name);
    PmError error = Pm_OpenInput(&g_midi_stream, my_id, nullptr, MAX_MIDI_EVENTS, nullptr, nullptr);
    if (error != 0) {
        LOGE("init_midi: error %d", error);
    }
    return error == 0;
}
void free_midi() {
    if (g_midi_stream) {
        Pm_Close(g_midi_stream);
        g_midi_stream = nullptr;
    }
    Pm_Terminate();
}


void main_loop() {

#ifdef __EMSCRIPTEN__
    {
        static int w, h;
        int ow = w, oh = h;
        emscripten_get_canvas_element_size("#canvas", &w, &h);
        if (w != ow || h != oh) SDL_SetWindowSize(g_window, w, h);
    }
#endif

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_QUIT:
            g_running = false;
            break;

        case SDL_KEYDOWN:
            switch (e.key.keysym.scancode) {
            case SDL_SCANCODE_ESCAPE:
                g_running = false;
                break;
            case SDL_SCANCODE_RETURN:
                app::key(gui::KEYCODE_ENTER, 0);
                break;
            case SDL_SCANCODE_BACKSPACE:
                app::key(gui::KEYCODE_DEL, 0);
                break;
            case SDL_SCANCODE_PRINTSCREEN:
                app::key(gui::KEYCODE_PRINTSCREEN, 0);
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
    SDL_GL_SwapWindow(g_window);
}


} // namespace




namespace platform {

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

std::vector<std::string> list_assets(std::string const& dir) {
    namespace fs = std::filesystem;
    std::vector<std::string> list;
    for (auto const& entry : fs::directory_iterator("assets/" + dir)) {
        if (!entry.is_regular_file()) continue;
        list.emplace_back(entry.path().filename().string());
    }
    std::sort(list.begin(), list.end());
    return list;
}

void show_keyboard(bool enabled) {
    SDL_StartTextInput();
}

void export_file(std::string const& path, std::string const& title, bool delete_when_done) {}
void start_song_import() {}
void update_setting(int i) {}

bool poll_midi_event(uint8_t& status, uint8_t& data1, uint8_t& data2) {
    if (!g_midi_stream) return false;
    static PmEvent events[MAX_MIDI_EVENTS];
    static int     pos = 0;
    static int     len = 0;
    if (pos >= len) {
        pos = 0;
        len = Pm_Read(g_midi_stream, events, MAX_MIDI_EVENTS);
        if (len <= 0) return false;
    }
    PmMessage msg = events[pos++].message;
    status = Pm_MessageStatus(msg);
    data1  = Pm_MessageData1(msg);
    data2  = Pm_MessageData2(msg);
    return true;
}

} // namespace platform


int main(int argc, char** argv) {

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);

    g_window = SDL_CreateWindow(
            "GTMobile",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            app::CANVAS_WIDTH, app::CANVAS_MIN_HEIGHT,
            SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    SDL_GLContext gl_context = SDL_GL_CreateContext(g_window);
    if (!gl_context) {
        LOGE("main: SDL_GL_CreateContext() failed");
        SDL_GL_DeleteContext(gl_context);
        SDL_DestroyWindow(g_window);
        SDL_Quit();
        return 1;
    }

    SDL_GL_SetSwapInterval(1);
    glewExperimental = true;
    glewInit();

    init_midi();
    start_audio();
    app::init();
    app::resize(app::CANVAS_WIDTH, app::CANVAS_MIN_HEIGHT);

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop, 0, 1);
#else
    while (g_running) main_loop();
#endif

    app::free();
    stop_audio();
    free_midi();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(g_window);
    SDL_Quit();
    return 0;
}
