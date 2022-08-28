#include "../include/xfiber.h"
#include <iostream>

auto main() -> int {
    XFiber xfiber;

    xfiber.CreateFiber([]() {
        std::cout << "Hello World" << std::endl;
    });

    xfiber.CreateFiber([]() {
        std::cout << "How are you?" << std::endl;
    });

    xfiber.Dispatch();

    return 0;
}