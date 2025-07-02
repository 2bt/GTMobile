#include "log.hpp"
#include "app.hpp"
#include "gui.hpp"
#include "settings_view.hpp"
#include "project_view.hpp"
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <jni.h>
#include <oboe/Oboe.h>
#include <vector>
#include <queue>
#include <mutex>


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

// midi
using MidiEvent = std::array<uint8_t, 3>;
std::queue<MidiEvent> g_midi_event_queue;
std::mutex            g_midi_queue_mutex;


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
    if (g_stream) return true;

    oboe::AudioStreamBuilder builder;
    builder.setDirection(oboe::Direction::Output);
//    builder.setPerformanceMode(oboe::PerformanceMode::LowLatency);
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

std::vector<std::string> list_assets(std::string const& dir) {
    AAssetDir* ad = AAssetManager_openDir(g_asset_manager, dir.c_str());
    if (!ad) {
        LOGE("list_assets: could not open %s", dir.c_str());
        return {};
    }
    std::vector<std::string> list;
    while (char const* s = AAssetDir_getNextFileName(ad)) {
        list.emplace_back(s);
    }
    AAssetDir_close(ad);
    return list;
}

void show_keyboard(bool enabled) {
    jclass clazz = g_env->FindClass("com/twobit/gtmobile/MainActivity");
    jmethodID method = g_env->GetStaticMethodID(clazz, "showKeyboard", "(Z)V");
    g_env->CallStaticVoidMethod(clazz, method, enabled);
}

void export_file(std::string const& path, std::string const& title, bool delete_when_done) {
    jclass clazz = g_env->FindClass("com/twobit/gtmobile/MainActivity");
    jmethodID method = g_env->GetStaticMethodID(clazz, "exportFile", "(Ljava/lang/String;Ljava/lang/String;Z)V");
    jstring jpath = g_env->NewStringUTF(path.c_str());
    jstring jtitle = g_env->NewStringUTF(title.c_str());
    g_env->CallStaticVoidMethod(clazz, method, jpath, jtitle, delete_when_done);
    g_env->DeleteLocalRef(jpath);
    g_env->DeleteLocalRef(jtitle);
}

void start_song_import() {
    jclass clazz = g_env->FindClass("com/twobit/gtmobile/MainActivity");
    jmethodID method = g_env->GetStaticMethodID(clazz, "startSongImport", "()V");
    g_env->CallStaticVoidMethod(clazz, method);
}

void update_setting(int i) {
    jclass clazz = g_env->FindClass("com/twobit/gtmobile/MainActivity");
    jmethodID method = g_env->GetStaticMethodID(clazz, "updateSetting", "(I)V");
    g_env->CallStaticVoidMethod(clazz, method, i);
}

bool poll_midi_event(uint8_t& status, uint8_t& data1, uint8_t& data2) {
    std::lock_guard<std::mutex> lock(g_midi_queue_mutex);
    if (g_midi_event_queue.empty()) return false;
    MidiEvent event = g_midi_event_queue.front();
    g_midi_event_queue.pop();
    status = event[0];
    data1  = event[1];
    data2  = event[2];
    return true;
}

} // namespace platform


extern "C" {
    JNIEXPORT void JNICALL Java_com_twobit_gtmobile_Native_init(
            JNIEnv* env,
            jclass,
            jobject asset_manager,
            jstring jstorage_dir,
            jfloat refresh_rate)
    {
        g_asset_manager = AAssetManager_fromJava(env, asset_manager);
        g_env = env;
        char const* storage_dir = env->GetStringUTFChars(jstorage_dir, nullptr);
        app::set_storage_dir(storage_dir);
        env->ReleaseStringUTFChars(jstorage_dir, storage_dir);
        app::init();
        gui::set_refresh_rate(refresh_rate);
    }
    JNIEXPORT void JNICALL Java_com_twobit_gtmobile_Native_free(JNIEnv* env, jclass) {
        g_env = env;
        app::free();
    }
    JNIEXPORT void JNICALL Java_com_twobit_gtmobile_Native_resize(JNIEnv* env, jclass, jint width, jint height) {
        g_env = env;
        app::resize(width, height);
    }
    JNIEXPORT void JNICALL Java_com_twobit_gtmobile_Native_setInsets(JNIEnv* env, jclass, jint topInset, jint bottomInset) {
        g_env = env;
        app::set_insets(topInset, bottomInset);
    }
    JNIEXPORT void JNICALL Java_com_twobit_gtmobile_Native_draw(JNIEnv* env, jclass) {
        g_env = env;
        app::draw();
    }
    JNIEXPORT void JNICALL Java_com_twobit_gtmobile_Native_touch(JNIEnv* env, jclass, jint x, jint y, jint action) {
        g_env = env;
        app::touch(x, y, action == 0 || action == 2);
    }
    JNIEXPORT void JNICALL Java_com_twobit_gtmobile_Native_key(JNIEnv* env, jclass, jint key, jint unicode) {
        g_env = env;
        app::key(key, unicode);
    }
    JNIEXPORT void JNICALL Java_com_twobit_gtmobile_Native_setPlaying(JNIEnv* env, jclass, jboolean stream, jboolean player) {
        LOGD("Native.setPlaying stream:%d player:%d", stream, player);
        g_env = env;
        if (player != app::player().is_playing()) {
            if (player) app::player().set_action(gt::Player::Action::Start);
            else {
                app::sid().reset(); // reset SID to make it silent
                app::player().set_action(gt::Player::Action::Pause);
            }
        }
        if (stream) start_audio();
        else stop_audio();
    }
    JNIEXPORT jboolean JNICALL Java_com_twobit_gtmobile_Native_isStreamPlaying(JNIEnv* env, jclass) {
        return g_stream != nullptr;
    }
    JNIEXPORT jboolean JNICALL Java_com_twobit_gtmobile_Native_isPlayerPlaying(JNIEnv* env, jclass) {
        return app::player().is_playing();
    }

    JNIEXPORT jstring JNICALL Java_com_twobit_gtmobile_Native_getSongName(JNIEnv* env, jclass) {
        return env->NewStringUTF(app::song().song_name.data());
    }

    JNIEXPORT jstring JNICALL Java_com_twobit_gtmobile_Native_getSettingName(JNIEnv* env, jclass, jint i) {
        char const* n = settings_view::get_setting_name(i);
        if (n) return env->NewStringUTF(n);
        return nullptr;
    }
    JNIEXPORT jint JNICALL Java_com_twobit_gtmobile_Native_getSettingValue(JNIEnv* env, jclass, jint i) {
        return settings_view::get_setting_value(i);
    }
    JNIEXPORT void JNICALL Java_com_twobit_gtmobile_Native_setSettingValue(JNIEnv* env, jclass, jint i, jint v) {
        settings_view::set_setting_value(i, v);
    }

    JNIEXPORT void JNICALL Java_com_twobit_gtmobile_Native_importSong(JNIEnv* env, jclass clazz, jstring jpath) {
        char const* path = env->GetStringUTFChars(jpath, nullptr);
        project_view::import_song(path);
        env->ReleaseStringUTFChars(jpath, path);
    }

    JNIEXPORT void JNICALL Java_com_twobit_gtmobile_Native_onMidiEvent(JNIEnv* env, jclass clazz, jbyteArray data, jint offset, jint count) {
        if (count < 3) return;
        MidiEvent event;
        env->GetByteArrayRegion(data, offset, 3, (jbyte*) event.data());
        std::lock_guard<std::mutex> lock(g_midi_queue_mutex);
        g_midi_event_queue.push(event);
    }

}
