#pragma once

#include <inttypes.h>
#include <sys/time.h>
#include <unistd.h>

namespace util {
    int64_t NowMs();
}