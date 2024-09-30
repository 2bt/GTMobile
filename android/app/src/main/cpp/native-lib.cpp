#include "log.hpp"
#include "app.hpp"
#include "gui.hpp"
#include "settings_view.hpp"

#include <vector>

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <jni.h>
#include <oboe/Oboe.h>



namespace {


struct Callback : oboe::AudioStreamCallback {
    oboe::DataCallbackResult onAudioReady(oboe::AudioStream* oboeStream, void* audioData, int32_t numFrames) override {
        app::audio_callback((int16_t*) audioData, numFrames);
        return oboe::DataCallbackResult::Continue;
    }
}                  g_callback;
oboe::AudioStream* g_stream;
AAssetManager*     g_asset_manager;
JNIEnv*            g_env;


bool start_audio() {
    LOGI("start_audio");
    if (g_stream) return true;

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

void stop_audio() {
    LOGI("stop_audio");
    if (!g_stream) return;
    g_stream->stop();
    g_stream->close();
    delete g_stream;
    g_stream = nullptr;
}


} // namespace


namespace platform {


bool load_asset(char const* name, std::vector<uint8_t>& buf) {
    AAsset* ad = AAssetManager_open(g_asset_manager, name, AASSET_MODE_BUFFER);
    if (!ad) {
        LOGE("load_asset: could not open %s", name);
        return false;
    }
    buf.resize(AAsset_getLength(ad));
    int len = AAsset_read(ad, buf.data(), buf.size());
    AAsset_close(ad);
    if (len != buf.size()) {
        LOGE("load_asset: could not open %s", name);
        return false;
    }

    return true;
}

bool list_assets(char const* dir, std::vector<std::string>& list) {
    list.clear();
    AAssetDir* ad = AAssetManager_openDir(g_asset_manager, dir);
    if (!ad) {
        LOGE("list_assets: could not open %s", dir);
        return false;
    }
    while (char const* s = AAssetDir_getNextFileName(ad)) {
        list.push_back(s);
    }
    AAssetDir_close(ad);
    return true;
}


void show_keyboard(bool enabled) {
    jclass clazz = g_env->FindClass("com/twobit/gtmobile/MainActivity");
    jmethodID method = g_env->GetStaticMethodID(clazz, "showKeyboard", "(Z)V");
    g_env->CallStaticVoidMethod(clazz, method, enabled);
}




} // namespace platform


extern "C" {
    JNIEXPORT void JNICALL Java_com_twobit_gtmobile_Native_init(JNIEnv* env, jobject thiz, jobject asset_manager, jfloat refresh_rate) {
        g_asset_manager = AAssetManager_fromJava(env, asset_manager);
        g_env = env;
        app::init();
        gui::set_refresh_rate(refresh_rate);
    }
    JNIEXPORT void JNICALL Java_com_twobit_gtmobile_Native_free(JNIEnv* env, jobject thiz) {
        g_env = env;
        app::free();
    }
    JNIEXPORT void JNICALL Java_com_twobit_gtmobile_Native_resize(JNIEnv* env, jobject thiz, jint width, jint height) {
        g_env = env;
        app::resize(width, height);
    }
    JNIEXPORT void JNICALL Java_com_twobit_gtmobile_Native_draw(JNIEnv* env, jobject thiz) {
        g_env = env;
        app::draw();
    }
    JNIEXPORT void JNICALL Java_com_twobit_gtmobile_Native_touch(JNIEnv* env, jobject thiz, jint x, jint y, jint action) {
        g_env = env;
        app::touch(x, y, action == 0 || action == 2);
    }
    JNIEXPORT void JNICALL Java_com_twobit_gtmobile_Native_key(JNIEnv* env, jobject thiz, jint key, jint unicode) {
        g_env = env;
        app::key(key, unicode);
    }
    JNIEXPORT void JNICALL Java_com_twobit_gtmobile_Native_startAudio(JNIEnv* env, jobject thiz) {
        g_env = env;
        start_audio();
    }
    JNIEXPORT void JNICALL Java_com_twobit_gtmobile_Native_stopAudio(JNIEnv* env, jobject thiz) {
        g_env = env;
        if (!settings_view::settings().play_in_background) {
            stop_audio();
        }
    }


    enum {
        #define X(n, ...) SETTING_##n,
        SETTINGS(X)
        #undef X
    };
    JNIEXPORT jstring JNICALL Java_com_twobit_gtmobile_Native_getValueName(JNIEnv* env, jobject thiz, jint i) {
        switch (i) {
        #define X(n, ...) case SETTING_##n: return env->NewStringUTF(#n);
        SETTINGS(X)
        #undef X
        default: return nullptr;
        }
    }
    JNIEXPORT jint JNICALL Java_com_twobit_gtmobile_Native_getValue(JNIEnv* env, jobject thiz, jint i) {
        switch (i) {
        #define X(n, ...) case SETTING_##n: return settings_view::settings().n;
        SETTINGS(X)
        #undef X
        default: return 0;
        }
    }
    JNIEXPORT void JNICALL Java_com_twobit_gtmobile_Native_setValue(JNIEnv* env, jobject thiz, jint i, jint v) {
        switch (i) {
        #define X(n, ...) case SETTING_##n: settings_view::mutable_settings().n = v; break;
        SETTINGS(X)
        #undef X
        default: break;
        }
    }
}
