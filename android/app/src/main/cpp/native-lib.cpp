#include <jni.h>
#include <string>
#include <oboe/Oboe.h>

#include "log.hpp"


enum {
    MIXRATE = 44100,
};


class Callback : public oboe::AudioStreamCallback {
public:
    oboe::DataCallbackResult onAudioReady(oboe::AudioStream* oboeStream, void* audioData, int32_t numFrames) override {

//        player::fill_buffer((short*) audioData, numFrames);

        static float phase = 0.0f;

        float* buffer = (float*) audioData;
        for (int i = 0; i < numFrames; ++i) {

            buffer[i] = sin(phase * M_PI_2) * 0.333f;
            phase += 440.0f / MIXRATE;
            phase -= (int) phase;
        }

        return oboe::DataCallbackResult::Continue;
    }
};


Callback           g_callback;
oboe::AudioStream* g_stream = nullptr;

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
//    builder.setFormat(oboe::AudioFormat::I16);
    builder.setFormat(oboe::AudioFormat::Float);
    builder.setChannelCount(oboe::ChannelCount::Mono);
    builder.setCallback(&g_callback);

    oboe::Result result = builder.openStream(&g_stream);
    if (result != oboe::Result::OK) {
        LOGE("start_audio: builder.openStream failed: %s", oboe::convertToText(result));
        return false;
    }

    oboe::AudioFormat format = g_stream->getFormat();
    LOGI("start_audio: stream format is %s", oboe::convertToText(format));

    //g_stream->setBufferSizeInFrames(s_stream->getFramesPerBurst() * 2);

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


extern "C" JNIEXPORT void JNICALL
Java_com_twobit_gtmobile_Native_init(
        JNIEnv* env,
        jobject /* this */) {

    start_audio();
}