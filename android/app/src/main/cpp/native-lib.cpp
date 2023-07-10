#include "gtplayer.hpp"
#include "log.hpp"
#include "sidengine.hpp"

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <jni.h>
#include <oboe/Oboe.h>
#include <vector>


enum {
    SAMPLES_PER_FRAME = MIXRATE / 50,
};


namespace {

gt::Song   g_song;
gt::Player g_player(g_song);
SidEngine  g_sid_engine;


class Callback : public oboe::AudioStreamCallback {
public:
    oboe::DataCallbackResult onAudioReady(oboe::AudioStream* oboeStream,
                                          void*              audioData,
                                          int32_t            numFrames) override {
        int16_t* buffer = (int16_t*) audioData;
        int      length = numFrames;

        while (length > 0) {
            if (m_sample == 0) {
                g_player.play_routine();
                for (int i = 0; i < 25; ++i) g_sid_engine.write(i, g_player.regs[i]);
            }
            int l = std::min(SAMPLES_PER_FRAME - m_sample, length);
            m_sample += l;
            if (m_sample == SAMPLES_PER_FRAME) m_sample = 0;
            length -= l;
            g_sid_engine.mix(buffer, l);
            buffer += l;
        }

        return oboe::DataCallbackResult::Continue;
    }
private:
    int m_sample = 0;
}                  g_callback;
oboe::AudioStream* g_stream = nullptr;

AAssetManager* g_asset_manager;


} // namespace


bool load_asset(std::string const& name, std::vector<uint8_t>& buf) {
    AAsset* ad = AAssetManager_open(g_asset_manager, name.c_str(), AASSET_MODE_BUFFER);
    if (!ad) {
        LOGE("load_asset: could not open %s", name.c_str());
        return false;
    }
    buf.resize(AAsset_getLength(ad));
    int len = AAsset_read(ad, buf.data(), buf.size());
    AAsset_close(ad);
    if (len != buf.size()) {
        LOGE("load_asset: could not open %s", name.c_str());
        return false;
    }
    return true;
}

void stop_audio() {
    LOGI("stop_audio");
    if (!g_stream) return;
    g_stream->stop();
    g_stream->close();
    delete g_stream;
    g_stream = nullptr;
}

bool start_audio() {
    LOGI("start_audio");
    if (g_stream) stop_audio();

    oboe::AudioStreamBuilder builder;
    builder.setDirection(oboe::Direction::Output);
    builder.setPerformanceMode(oboe::PerformanceMode::LowLatency);
    builder.setSharingMode(oboe::SharingMode::Exclusive);
    builder.setSampleRate(MIXRATE);
    builder.setFormat(oboe::AudioFormat::I16);
    builder.setChannelCount(oboe::ChannelCount::Mono);
    builder.setCallback(&g_callback);

    oboe::Result result = builder.openStream(&g_stream);
    if (result != oboe::Result::OK) {
        LOGE("start_audio: builder.openStream failed: %s", oboe::convertToText(result));
        return false;
    }

    oboe::AudioFormat format = g_stream->getFormat();
    LOGI("start_audio: stream format is %s", oboe::convertToText(format));

    auto rate = g_stream->getSampleRate();
    if (rate != MIXRATE) {
        LOGW("start_audio: mixrate is %d but should be %d", rate, MIXRATE);
    }

    result = g_stream->requestStart();
    if (result != oboe::Result::OK) {
        LOGE("start_audio: AudioStream::requestStart failed: %s", oboe::convertToText(result));
        return false;
    }
    return true;
}


extern "C" JNIEXPORT void JNICALL Java_com_twobit_gtmobile_Native_init(JNIEnv* env,
                                                                       jobject thiz,
                                                                       jobject asset_manager) {
    g_asset_manager = AAssetManager_fromJava(env, asset_manager);

    // load song
    std::vector<uint8_t> buffer;
    load_asset("Remembrance.sng", buffer);
    struct MemBuf : std::streambuf {
        MemBuf(uint8_t const* data, size_t size) {
            char* p = (char*)data;
            this->setg(p, p, p + size);
        }
    } membuf(buffer.data(), buffer.size());
    std::istream stream(&membuf);
    g_song.load(stream);
    g_player.init_song(0, gt::Player::PLAY_BEGINNING);


    start_audio();
}
