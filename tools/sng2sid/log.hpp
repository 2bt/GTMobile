#pragma once
#include <cstdio>
#define LOGI(fmt, ...) printf(          fmt "\n", ##__VA_ARGS__)
#define LOGD(fmt, ...) printf("DEBUG: " fmt "\n", ##__VA_ARGS__)
#define LOGW(fmt, ...) printf("WARN:  " fmt "\n", ##__VA_ARGS__)
#define LOGE(fmt, ...) printf("ERROR: " fmt "\n", ##__VA_ARGS__)
