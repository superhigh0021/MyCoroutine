#pragma once

#include <cstdio>
#include <string>

#define DEBUG_ENABLE (1 << 0)
#define INFO_ENABLE (1 << 1)
#define WARNING_ENABLE (1 << 2)
#define ERROR_ENABLE (1 << 3)

static std::string log_date() {
    time_t now = time(0);
    tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);
    return buf;
}

#ifndef __LOG_DATE__
#define __LOG_DATE__ log_date().c_str()
#endif

#if DEBUG_ENABLE
#define LOGD(fmt, args...) fprintf(stderr, "[D][%s][%s %d] " fmt "\n", __LOG_DATE__, __FILE__, __LINE__, ##args);
#else
#define LOGD(fmt, ...)
#endif

#if INFO_ENABLE
#define LOGI(fmt, args...) fprintf(stderr, "[I][%s][%s %d] " fmt "\n", __LOG_DATE__, __FILE__, __LINE__, ##args);
#else
#define LOGI(fmt, ...)
#endif

#if WARNING_ENABLE
#define LOGW(fmt, args...) fprintf(stderr, "[W][%s][%s %d] " fmt "\n", __LOG_DATE__, __FILE__, __LINE__, ##args);
#else
#define LOGW(fmt, ...)
#endif

#if ERROR_ENABLE
#define LOGE(fmt, args...) fprintf(stderr, "[E][%s][%s %d] " fmt "\n", __LOG_DATE__, __FILE__, __LINE__, ##args);
#else
#define LOGE(fmt, ...)
#endif