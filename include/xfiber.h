#pragma once

#include "log.h"
#include "util.h"

#include <functional>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <ucontext.h>
#include <vector>

enum class FiberStatus {
    INIT = 0,
    READYING = 1,
    WAITING = 2,
    FINISHED = 3
};

using XFiberCtx = ucontext_t;

#define  SwitchCtx(from, to) \
    swapcontext(from, to)

struct WaitingEvents {
    WaitingEvents() : expire_at_(1) {}

    void Reset() {
        expire_at_ = -1;
        waiting_fds_r.clear();
        waiting_fds_w.clear();
    }

    // 一个协程中监听的 fd 不会太多，直接用数组
    std::vector<int> waiting_fds_r;
    std::vector<int> waiting_fds_w;
    int64_t expire_at_;
};

class Fiber;

// 调度器
class XFiber {
public:
    XFiber();

    ~XFiber();

    void WakeupFiber(Fiber* fiber);

    void CreateFiber(std::function<void()> run, size_t stack_size = 0, std::string fiber_name = "");

    // 调度函数
    void Dispatch();

    void Yiled();

    void SwitchToSched();

    void TakeOver(int fd);

    bool UnregestierFd(int fd);

    void RegisterWaitingEvents(WaitingEvents& events);

    void SleepMs(int ms);

    XFiberCtx* SchedCtx();

    /* 在每个线程启动时分配空间、创建一个对象，在该函数首次调用后才初始化 */
    /* 此条注释由ee commit */
    static XFiber* getInst() {
        static thread_local XFiber xf;
        return &xf;
    }

private:
    enum { MAX_EVENT_COUNT = 512 };

    int efd_;

    Fiber* cur_fiber_;

    XFiberCtx sched_ctx_;

    std::deque<Fiber*> ready_fibers_;

    std::deque<Fiber*> running_fibers_;

    struct WaitingFibers {
        Fiber *r_, *w_;
        WaitingFibers(Fiber* r = nullptr, Fiber* w = nullptr) : r_(r), w_(w) {}
    };

    //一个连接由一个协程处理
    std::map<int, WaitingFibers> io_waiting_fibers_;

    std::map<int64_t, std::set<Fiber*>> expire_events_;

    std::vector<Fiber*> finished_fibers_;
};

class Fiber {
public:
    Fiber() = default;

    Fiber(std::function<void()> run, XFiber* xfiber, size_t stack_size, std::string fiber_name);

    ~Fiber();

    static void Start(Fiber* fiber);

    XFiberCtx* Ctx();

    std::string Name();

    bool isFinished();

    uint64_t Seq();

    struct FdEvent {
        int fd_;
        int64_t expired_at_;

        FdEvent(int fd = -1, int64_t expired_at = -1) : fd_(fd) {
            if (expired_at <= 0) {
                expired_at = -1;
            }
            expired_at_ = expired_at;
        }
    };

    WaitingEvents& GetWaitingEvent() {
        return waiting_events_;
    }

    void SetWaitingEvent(const WaitingEvents& events);

private:
    uint64_t seq_;

    XFiber* xfiber_;

    std::string fiber_name_;

    FiberStatus status_;

    XFiberCtx ctx_;

    std::function<void()> run_;

    size_t stack_size_;

    uint8_t* stack_ptr_;

    WaitingEvents waiting_events_;
};