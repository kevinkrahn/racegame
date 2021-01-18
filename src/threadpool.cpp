#include "threadpool.h"

i32 threadFunc(void* data)
{
	ThreadPool* threadpool = (ThreadPool*)data;
	threadpool->worker();
	return 0;
}

void ThreadPool::start()
{
	//u32 numThreads = (u32)sysconf(_SC_NPROCESSORS_ONLN);
    u32 numThreads = 4;
    threads.resize(numThreads);
	const char* threadNames[] = { "Worker1", "Worker2", "Worker3", "Worker4" };
	for (u32 i = 0; i < numThreads; ++i)
	{
	    threads[i] = SDL_CreateThread(threadFunc, threadNames[i], this);
	}
}

void ThreadPool::signalCompletion()
{
	isFinished = true;
	SDL_CondBroadcast(wakeCond);
}

void ThreadPool::join()
{
	for (SDL_Thread* t : threads)
	{
	    SDL_WaitThread(t, nullptr);
	}
}

void ThreadPool::addTask(Task const&& task)
{
    SDL_LockMutex(taskMtx);
	tasks.push(task);
	SDL_UnlockMutex(taskMtx);
	SDL_CondSignal(wakeCond);
}

void ThreadPool::wait()
{
    if (tasks.empty())
    {
        return;
    }
	SDL_LockMutex(waitMtx);
	SDL_CondWait(waitCond, wakeMtx);
	SDL_UnlockMutex(waitMtx);
}

void ThreadPool::worker()
{
	while (!isFinished)
	{
	    SDL_LockMutex(wakeMtx);
	    SDL_CondWait(wakeCond, wakeMtx);
	    SDL_UnlockMutex(wakeMtx);

	    while (!tasks.empty())
	    {
            Task task;
            {
	            SDL_LockMutex(taskMtx);
                if (!tasks.empty())
                {
#if 0
                    task = tasks.front();
                    tasks.erase(tasks.begin());
#else
                    task = tasks.back();
                    tasks.pop();
#endif
                }
                SDL_UnlockMutex(taskMtx);
            }
            if (task.execute)
            {
                task.execute(task.data);
            }
	    }

	    SDL_LockMutex(waitMtx);
	    SDL_CondSignal(waitCond);
	    SDL_UnlockMutex(waitMtx);
	}
}
