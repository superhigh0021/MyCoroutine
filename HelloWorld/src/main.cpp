#include "xfiber.h"
#include "xsocket.h"
#include <csignal>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <unistd.h>

void sigintAction(int sig) {
    std::cout << "exit..." << std::endl;
    exit(0);
}

// #define DEBUG_ENABLE
#define DPRINT(fmt, args...) fprintf(stderr, "[D][%s %d] " fmt "\n", __FILE__, __LINE__, ##args);

auto main() -> int {
    signal(SIGINT, sigintAction);
    XFiber* xfiber = XFiber::getInst();

    xfiber->CreateFiber([&] {
        Listener listener = Listener::ListenTCP(7000);
        while (true) {
            std::shared_ptr<Connection> conn1 = listener.Accept();
            // shared_ptr<Connection> conn2 = Connection::ConnectTCP("127.0.0.1", 6379);

            xfiber->CreateFiber([conn1] {
                while (true) {
                    char recv_buf[512];
                    int n = conn1->Read(recv_buf, 512, 50000);
                    if (n <= 0) {
                        break;
                    }

#if 0
                    conn2->Write(recv_buf, n);
                    char rsp[1024];
                    int rsp_len = conn2->Read(rsp, 1024);
                    cout << "recv from remote: " << rsp << endl;
                    conn1->Write(rsp, rsp_len);
#else
                    if (conn1->Write("+OK\r\n", 5, 1000) <= 0) {
                        break;
                    }
#endif
                }
            },
                                0, "server");
        }
    });

    xfiber->Dispatch();

    return 0;
}