#include "log.hpp"
#include "app.hpp"

#include <vector>

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <jni.h>
#include <oboe/Oboe.h>



namespace {


struct Callback : oboe::AudioStreamCallback {
    oboe::DataCallbackResult onAudioReady(oboe::AudioStream* oboeStream,
                                          void*              audioData,
                                          int32_t            numFrames) override {
        app::audio_callback((int16_t*) audioData, numFrames);

        return oboe::DataCallbackResult::Continue;
    }
}                  g_callback;
oboe::AudioStream* g_stream;
AAssetManager*     g_asset_manager;


} // namespace


namespace platform {


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
    builder.setSampleRate(app::MIXRATE);
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
    if (rate != app::MIXRATE) {
        LOGW("start_audio: mixrate is %d but should be %d", rate, app::MIXRATE);
    }

    result = g_stream->requestStart();
    if (result != oboe::Result::OK) {
        LOGE("start_audio: AudioStream::requestStart failed: %s", oboe::convertToText(result));
        return false;
    }
    return true;
}


} // namespace platform


extern "C" {
    JNIEXPORT void JNICALL Java_com_twobit_gtmobile_Native_init(JNIEnv* env, jobject thiz, jobject asset_manager) {
        g_asset_manager = AAssetManager_fromJava(env, asset_manager);
        app::init();
    }
    JNIEXPORT void JNICALL Java_com_twobit_gtmobile_Native_free(JNIEnv* env, jobject thiz) {
        app::free();
    }

}
