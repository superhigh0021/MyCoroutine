#include <stdio.h>
#include <ucontext.h>
#include <unistd.h>

void func1() {
    puts("1");
    puts("11");
    puts("111");
    puts("1111");
}

void context_test() {
    char stack[1024 * 128] = {0};
    ucontext_t child, main;

    getcontext(&child);
    child.uc_stack.ss_sp = stack;
    child.uc_stack.ss_size = sizeof(stack);
    child.uc_stack.ss_flags = 0;
    child.uc_link = NULL;
    // child.uc_link = &main;

    makecontext(&child, func1, 0);

    swapcontext(&main, &child);
    puts("main");
}

auto main(int argc, char* argv[]) -> int {
    context_test();

    return 0;
}