#pragma once

#include <functional>
#include <list>
#include <ucontext.h>

class Fiber;

class XFiber {
public:
    XFiber();

    ~XFiber();

    void CreateFiber(std::function<void()> run);

    void Dispatch();

    void Yiled();

    ucontext_t* SchedCtx();

private:
    ucontext_t sched_ctx_;

    std::list<Fiber*> ready_fibers_, running_fibers_;
};

class Fiber {
public:
    Fiber() = default;

    Fiber(std::function<void()> run, XFiber* Xfiber);

    ~Fiber();

    static void Start(Fiber* fiber);

    ucontext_t* Ctx();

    bool isFinished();

private:
    int status_;
    
    ucontext_t ctx_;

    std::function<void()> run_;

    char* stack_;

    size_t stack_size_;
};