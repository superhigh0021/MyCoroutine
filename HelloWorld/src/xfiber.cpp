#include "../include/xfiber.h"
#include <iostream>

XFiber::XFiber() {}

XFiber::~XFiber() {}

void XFiber::CreateFiber(std::function<void()> run) {
    ready_fibers_.emplace_back(new Fiber{run, this});
}

ucontext_t* XFiber::SchedCtx() {
    return &sched_ctx_;
}

void XFiber::Dispatch() {
    while (true) {
        if (ready_fibers_.empty()) {
            continue;
        }

        running_fibers_ = std::move(ready_fibers_);
        ready_fibers_.clear();
        std::cout << "size = " << running_fibers_.size() << std::endl;
        for (auto i = running_fibers_.begin(); i != running_fibers_.end(); ++i) {
            Fiber* fiber = *i;
            swapcontext(&sched_ctx_, fiber->Ctx());

            if (fiber->isFinished()) {
                delete fiber;
            }
        }
        running_fibers_.clear();
    }
}

Fiber::Fiber(std::function<void()> run, XFiber* xfiber) : run_(run), status_(0) {
    getcontext(&ctx_);
    stack_size_ = 1024 * 128;
    stack_ = new char[stack_size_];
    ctx_.uc_link = xfiber->SchedCtx();
    ctx_.uc_stack.ss_size = stack_size_;
    ctx_.uc_stack.ss_sp = stack_;

    makecontext(&ctx_, (void (*)())Fiber::Start, 1, this);
}

Fiber::~Fiber() {
    delete stack_;
    stack_ = nullptr;
    stack_size_ = 0;
}

void Fiber::Start(Fiber* fiber) {
    std::cout<<"before Start"<<std::endl;
    fiber->run_();
    std::cout<<"after Start"<<std::endl;
    fiber->status_ = -1;
}

bool Fiber::isFinished() {
    return status_ == -1;
}

ucontext_t* Fiber::Ctx() {
    return &ctx_;
}