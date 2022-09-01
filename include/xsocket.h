#pragma once

#include "util.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <inttypes.h>
#include <memory>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

class Fd {
public:
    Fd();

    ~Fd() = default;

    static uint32_t next_seq_;

    int RawFd();

    void RegisterFdToSched();

    bool Available();

    int pub;

protected:
    int fd_;

    int seq_;
};

class Connection;

class Listener : public Fd {
public:
    Listener() = default;

    ~Listener();

    std::shared_ptr<Connection> Accept();

    void setFd(int fd);

    static Listener ListenTCP(uint16_t port);

private:
    uint16_t port_;
};

class Connection : public Fd {
public:
    Connection() = default;

    Connection(int fd);

    ~Connection();

    static std::shared_ptr<Connection> ConnectTCP(const char* ipv4, uint16_t port);

    ssize_t Write(const char* buf, size_t sz, int timeout_ms = -1) const;

    ssize_t Read(char* buf, size_t sz, int timeout_ms = -1) const;
};