#include "app.hpp"
#include "platform.hpp"
#include "gtplayer.hpp"
#include "sidengine.hpp"


namespace app {
namespace {

gt::Song   g_song;
gt::Player g_player(g_song);
SidEngine  g_sid_engine(MIXRATE);


} // namespace


void audio_callback(int16_t* buffer, int length) {
    enum {
        SAMPLES_PER_FRAME = MIXRATE / 50,
    };
    static int sample = 0;
    while (length > 0) {
        if (sample == 0) {
            g_player.play_routine();
            for (int i = 0; i < 25; ++i) g_sid_engine.write(i, g_player.regs[i]);
        }
        int l = std::min(SAMPLES_PER_FRAME - sample, length);
        sample += l;
        if (sample == SAMPLES_PER_FRAME) sample = 0;
        length -= l;
        g_sid_engine.mix(buffer, l);
        buffer += l;
    }
}

void init() {

    // load song
    std::vector<uint8_t> buffer;
    platform::load_asset("Remembrance.sng", buffer);
    struct MemBuf : std::streambuf {
        MemBuf(uint8_t const* data, size_t size) {
            char* p = (char*)data;
            this->setg(p, p, p + size);
        }
    } membuf(buffer.data(), buffer.size());
    std::istream stream(&membuf);
    g_song.load(stream);
    g_player.init_song(0, gt::Player::PLAY_BEGINNING);


    platform::start_audio();
}

void free() {


}


} // namespace app
