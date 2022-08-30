#include "../include/xfiber.h"
#include "xsocket.h"
#include <iostream>
#include <unistd.h>

auto main() -> int {
    XFiber xfiber;

    xfiber.CreateFiber([]() {
        std::cout << "Hello World" << std::endl;
    });

    xfiber.CreateFiber([]() {
        std::cout << "How are you?" << std::endl;
    });

    xfiber.CreateFiber([&]() {
        //listen
        Listener listener;
        listener.ListenTCP(8888);
        // accept
        while (true){
        int client_fd = listener.Accept();
        if (client_fd < 0) {
            continue;
        }

        xfiber.CreateFiber([&]() {
            char buf[32];
            int n = read(client_fd, buf, 32);
            printf("recv: %s\n", buf);
            write(client_fd, buf, n);
        });
        xfiber.Yiled();
    } });

    xfiber.Dispatch();

    return 0;
}