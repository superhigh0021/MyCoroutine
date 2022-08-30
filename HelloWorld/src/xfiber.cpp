#include "xfiber.h"

#include <cassert>
#include <cstring>
#include <errno.h>
#include <error.h>
#include <iostream>
#include <signal.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <unistd.h>

XFiber::XFiber() : cur_fiber_(nullptr) {
    efd_ = epoll_create1(0);
    if (efd_ < 0) {
        LOGE("epoll_create failed, msg = %s", strerror(errno));
        exit(-1);
    }
}

XFiber::~XFiber() {
    close(efd_);
}

void XFiber::CreateFiber(std::function<void()> run, size_t stack_size, std::string fiber_name) {
    if (stack_size == 0) {
        stack_size = 1024 * 1024;
    }
    Fiber* fiber = new Fiber(run, this, stack_size, fiber_name);
    ready_fibers_.push_back(fiber);
    LOGD("create a new fiber with id[%lu]", fiber->Seq());
}

ucontext_t* XFiber::SchedCtx() {
    return &sched_ctx_;
}

void XFiber::WakeupFiber(Fiber* fiber) {
    LOGD("try wakeup fiber[%lu] %p", fiber->Seq(), fiber);

    // 1. 加入就绪队列
    ready_fibers_.emplace_back(fiber);

    // 2. 从等待队列中删除
    WaitingEvents& waiting_events = fiber->GetWaitingEvent();
    for (int fdRead : waiting_events.waiting_fds_r) {
        auto iter = io_waiting_fibers_.find(fdRead);
        if (iter != io_waiting_fibers_.end()) {
            io_waiting_fibers_.erase(iter);
        }
    }

    for (int fdWrite : waiting_events.waiting_fds_w) {
        auto iter = io_waiting_fibers_.find(fdWrite);
        if (iter != io_waiting_fibers_.end()) {
            io_waiting_fibers_.erase(iter);
        }
    }

    // 3. 从超时队列中删除
    int64_t expire_at = waiting_events.expire_at_;
    if (expire_at > 0) {
        auto expire_iter = expire_events_.find(expire_at);
        if (expire_iter->second.find(fiber) == expire_iter->second.end()) {
            LOGW("not fiber [%lu] om expired events", fiber->Seq());
        } else {
            LOGD("remove fiber [%lu] from expire events...", fiber->Seq());
            expire_iter->second.erase(fiber);
        }
    }
    LOGD("fiber [%lu] %p has wakeup success, ready to run!", fiber->Seq(), fiber);
}

void XFiber::Dispatch() {
    while (true) {
        if (!ready_fibers_.empty()) {
            running_fibers_ = std::move(ready_fibers_);
            ready_fibers_.clear();
            LOGD("there are %ld fiber(s) in ready list, ready to run...", running_fibers_.size());

            for (auto fiber : running_fibers_) {
                cur_fiber_ = fiber;
                LOGD("switch from sched to fiber[%lu]", fiber->Seq());
                assert(SwitchCtx(SchedCtx(), fiber->Ctx()) == 0);
                cur_fiber_ = nullptr;

                if (fiber->isFinished()) {
                    LOGI("fiber[%lu] finished, free it!", fiber->Seq());
                    delete fiber;
                }
            }
            running_fibers_.clear();
        }

        int64_t now_ms = util::NowMs();
        while (!expire_events_.empty() && expire_events_.begin()->first <= now_ms) {
            //! 注意使用的是引用
            std::set<Fiber*>& expired_fibers = expire_events_.begin()->second;
            while (!expired_fibers.empty()) {
                auto expired_fiber = expired_fibers.begin();
                WakeupFiber(*expired_fiber);
            }
            expire_events_.erase(expire_events_.begin());
        }

        epoll_event evs[MAX_EVENT_COUNT];
        int n = epoll_wait(efd_, evs, MAX_EVENT_COUNT, 2);
        if (n < 0) {
            LOGE("epoll_wait error, msg=%s", strerror(errno));
            continue;
        }

        for (int i = 0; i < n; ++i) {
            epoll_event& ev = evs[i];
            int fd = ev.data.fd;

            auto fiber_iter = io_waiting_fibers_.find(fd);
            if (fiber_iter != io_waiting_fibers_.end()) {
                WaitingFibers& waiting_fiber = fiber_iter->second;
                if (ev.events & EPOLLIN) {
                    LOGD("waiting fd[%d] has fired IN event, wake up pending fiber[%lu]", fd, waiting_fiber.r_->Seq());
                    WakeupFiber(waiting_fiber.r_);
                } else if (ev.events & EPOLLOUT) {
                    if (waiting_fiber.w_ == nullptr) {
                        LOGW("fd[%d] has been fired OUT event, but not found any fiber to handle!", fd);
                    } else {
                        LOGD("waiting fd[%d] has fired OUT event, wake up pending fiber[%lu]", fd, waiting_fiber.w_->Seq());
                        WakeupFiber(waiting_fiber.w_);
                    }
                }
            }
        }
    }
}

void XFiber::Yiled() {
    assert(cur_fiber_ != nullptr);
    //! 主动切出的后仍然是 ready 状态，等待下次调度
    ready_fibers_.push_back(cur_fiber_);
    SwitchToSched();
}

void XFiber::SwitchToSched() {
    assert(cur_fiber_ != nullptr);
    LOGD("switch to sched");
    assert(SwitchCtx(cur_fiber_->Ctx(), SchedCtx()));
}

void XFiber::SleepMs(int ms) {
    if (ms < 0) {
        return;
    }

    int64_t expire_at = util::NowMs() + ms;
    WaitingEvents events;
    events.expire_at_ = expire_at;
    RegisterWaitingEvents(events);
    SwitchToSched();
}

void XFiber::TakeOver(int fd) {
    epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
    ev.data.fd = fd;

    if (epoll_ctl(efd_, EPOLL_CTL_ADD, fd, &ev) < 0) {
        LOGE("add fd [%d] into epoll failed, msg=%s", fd, strerror(errno));
        exit(-1);
    }
    LOGD("add fd[%d] into epoll event success", fd);
}

void XFiber::RegisterWaitingEvents(WaitingEvents& events) {
    assert(cur_fiber_ != nullptr);
    if (events.expire_at_ > 0) {
        expire_events_[events.expire_at_].insert(cur_fiber_);
        cur_fiber_->SetWaitingEvent(events);
        LOGD("register fiber [%lu] with expire event at %ld", cur_fiber_->Seq(), events.expire_at_);
    }

    for (int fdRead : events.waiting_fds_r) {
        auto iter = io_waiting_fibers_.find(fdRead);
        if (iter == io_waiting_fibers_.end()) {
            io_waiting_fibers_.insert(std::make_pair(fdRead, WaitingFibers(cur_fiber_, nullptr)));
            cur_fiber_->SetWaitingEvent(events);
        }
    }

    for (int fdWrite : events.waiting_fds_w) {
        auto iter = io_waiting_fibers_.find(fdWrite);
        if (iter == io_waiting_fibers_.end()) {
            io_waiting_fibers_.insert(std::make_pair(fdWrite, WaitingFibers(nullptr, cur_fiber_)));
            cur_fiber_->SetWaitingEvent(events);
        }
    }
}

bool XFiber::UnregestierFd(int fd) {
    LOGD("unregister fd[%d] from sheduler", fd);
    auto io_waiting_fibers_iter = io_waiting_fibers_.find(fd);

    if (io_waiting_fibers_iter != io_waiting_fibers_.end()) {
        WaitingFibers& waiting_fibers = io_waiting_fibers_iter->second;
        if (waiting_fibers.r_ != nullptr) {
            WakeupFiber(waiting_fibers.r_);
        }
        if (waiting_fibers.w_ != nullptr) {
            WakeupFiber(waiting_fibers.w_);
        }

        io_waiting_fibers_.erase(io_waiting_fibers_iter);
    }

    epoll_event ev;
    if (epoll_ctl(efd_, EPOLL_CTL_DEL, fd, &ev) < 0) {
        LOGE("unregister fd[%d] from epoll efd[%d] failed, msg=%s", fd, efd_, strerror(errno));
    } else {
        LOGI("unregister fd[%d] from epoll efd[%d] success!", fd, efd_);
    }
    return true;
}

thread_local uint64_t fiber_seq = 0;

Fiber::Fiber(std::function<void()> run,
             XFiber* xfiber,
             size_t stack_size,
             std::string fiber_name) : run_(run), xfiber_(xfiber), fiber_name_(fiber_name),
                                       stack_size_(stack_size), stack_ptr_(new uint8_t[stack_size_]) {
    getcontext(&ctx_);
    ctx_.uc_link = xfiber->SchedCtx();
    ctx_.uc_stack.ss_size = stack_size_;
    ctx_.uc_stack.ss_sp = stack_ptr_;
    makecontext(&ctx_, (void (*)())Fiber::Start, 1, this);

    seq_ = ++fiber_seq;
    status_ = FiberStatus::INIT;
}

Fiber::~Fiber() {
    delete stack_ptr_;
    stack_ptr_ = nullptr;
    stack_size_ = 0;
}

void Fiber::Start(Fiber* fiber) {
    fiber->run_();
    fiber->status_ = FiberStatus::FINISHED;
    LOGD("fiber[%lu] finished...", fiber->Seq());
}

std::string Fiber::Name() {
    return fiber_name_;
}

bool Fiber::isFinished() {
    return status_ == FiberStatus::FINISHED;
}

ucontext_t* Fiber::Ctx() {
    return &ctx_;
}

void Fiber::SetWaitingEvent(const WaitingEvents& events) {
    for (int fdRead : events.waiting_fds_r) {
        waiting_events_.waiting_fds_r.emplace_back(fdRead);
    }

    for (int fdWrite : events.waiting_fds_w) {
        waiting_events_.waiting_fds_w.emplace_back(fdWrite);
    }

    if (events.expire_at_ > 0) {
        waiting_events_.expire_at_ = events.expire_at_;
    }
}