#pragma once

#include "misc.h"

class Mutex
{
	SDL_mutex* mtx;
public:
    Mutex() { mtx = SDL_CreateMutex(); }
    ~Mutex() { SDL_DestroyMutex(mtx); }
	void lock() { SDL_LockMutex(mtx); }
	void unlock() { SDL_UnlockMutex(mtx); }
};

class LockGuard
{
    Mutex& mutex;

public:
    LockGuard(Mutex& mutex) : mutex(mutex)
    {
        mutex.lock();
    }
    ~LockGuard()
    {
        mutex.unlock();
    }
};

struct Task
{
    void* data = nullptr;
    void* (*execute)(void*) = nullptr;
};

class ThreadPool
{
    Array<Task> tasks;
    Array<SDL_Thread*> threads;

	SDL_mutex* wakeMtx;
    SDL_mutex* taskMtx;
    SDL_mutex* waitMtx;
	SDL_cond* wakeCond;
	SDL_cond* waitCond;

    bool isFinished = false;

    void worker();

public:
    ThreadPool()
    {
        wakeMtx = SDL_CreateMutex();
        taskMtx = SDL_CreateMutex();
        waitMtx = SDL_CreateMutex();
        wakeCond = SDL_CreateCond();
        waitCond = SDL_CreateCond();
    }
    ~ThreadPool()
    {
        SDL_DestroyMutex(wakeMtx);
        SDL_DestroyMutex(taskMtx);
        SDL_DestroyMutex(waitMtx);
        SDL_DestroyCond(wakeCond);
        SDL_DestroyCond(waitCond);
    }

    void start();
    void addTask(Task const&& task);
    void signalCompletion();
    void join();
    void wait();

	friend i32 threadFunc(void* data);
};

ThreadPool g_threadPool;
