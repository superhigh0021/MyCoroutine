#pragma once
#include <sys/types.h>
#include <inttypes.h>


class Listener {
public:
    void ListenTCP(uint16_t port);

    int Accept();
private:
    int fd_;
};