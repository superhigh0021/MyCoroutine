#ifndef MY_UTHREAD_H
#define MY_UTHREAD_H

#ifdef __APPLE__
#define _XOPEN_SOURCE
#endif

#include <iostream>
#include <ucontext.h>
#include <vector>

#define DEFAULT_STACK_SIZE (1024 * 128)
#define MAX_UTHREAD_SIZE 1024

enum ThreadState { FREE,
                   RUNNABLE,
                   EUNNING,
                   SUSPEND };

struct schedule_t;

using Fun = void (*)(void* arg);

struct uthread_t {
    ucontext_t ctx;
    Fun func;
    void* arg;
    enum ThreadState state;
    char stack[DEFAULT_STACK_SIZE];
};

struct schedule_t {
    ucontext_t main;
    int running_thread;
    uthread_t* threads;
    int max_index; //!曾经使用到的最大的 index + 1

    schedule_t() : running_thread(-1), max_index(0) {
        threads = new uthread_t[MAX_UTHREAD_SIZE];
        for (int i = 0; i < MAX_UTHREAD_SIZE; ++i) {
            threads[i].state = FREE;
        }
    }

    ~schedule_t() {
        delete[] threads;
    }
};

// help the thread running in the schedule
static void uthread_body(schedule_t* ps);

// create a user thread
int uthread_create(schedule_t& schedule, Fun func, void* arg);

// hang the currently running thread, switch to main thread
void uthread_yield(schedule_t& schedule);

// resume the thread which index == id
void uthread_resume(schedule_t& schedule, int id);

// test whether all the threads in schedule run over
int schedule_finished(const schedule_t& schedule);

#endif