#include "xsocket.h"
#include <arpa/inet.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

void Listener::ListenTCP(uint16_t port) {
    fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_ < 0) {
        perror("socket");
        exit(0);
    }

    if (fcntl(fd_, F_SETFL, O_NONBLOCK) < 0) {
        perror("fcntl");
        exit(0);
    }

    struct sockaddr_in addr;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = AF_INET;

    if (bind(fd_, (struct sockaddr*)&addr, sizeof(addr))) {
        perror("bind");
        exit(0);
    }

    if (listen(fd_, 32) < 0) {
        perror("listen");
        exit(0);
    }

    printf("Listen %d success\n", port);
}

int Listener::Accept() {
    while (true) {
        int client_fd = accept(fd_, nullptr, nullptr);
        if(client_fd > 0) {
            printf("accept success, fd = %d\n", client_fd);
            return client_fd;
        } else if (errno == EAGAIN) {
            //yield
        } else if(errno == EINTR) {
            continue;
        }else {
            perror("accept");
            return -1;
        }
    }
}