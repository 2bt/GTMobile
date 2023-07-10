#pragma once

#ifdef ANDROID
#include <android/log.h>
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO,  "jni", __VA_ARGS__))
#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, "jni", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN,  "jni", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "jni", __VA_ARGS__))
#else
#include <cstdio>
#define LOGI(fmt, ...) printf(          fmt "\n", ##__VA_ARGS__)
#define LOGD(fmt, ...) printf("DEBUG: " fmt "\n", ##__VA_ARGS__)
#define LOGW(fmt, ...) printf("WARN:  " fmt "\n", ##__VA_ARGS__)
#define LOGE(fmt, ...) printf("ERROR: " fmt "\n", ##__VA_ARGS__)
#endif
