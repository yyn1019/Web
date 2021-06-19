

#pragma once

#include<vector>
#include<list>
#include<functional>
#include<pthread.h>
#include<memory>

#include"noncopyable.h"
#include"MutexLock.h"
#include"Condition.h"

const int MAX_THREAD_SIZE = 1024;
const int MAX_QUEUE_SIZE = 10000;

typedef enum{
    immediate_mode = 1,
    graceful_mode = 2
} ShutdownMode;

struct ThreadTask
{
    std::function<void(std::shared_ptr<void>)> process;
    std::shared_ptr<void> arg;
};

class ThreadPool
{
private:
    //线程同步互斥，mutex_在condition 前面
    MutexLock mutex_;
    Condition condition_;
    //线程池属性
    int thread_size;
    int max_queue_size;
    int started;
    int shutdown_;
    std::vector<pthread_t> threads;
    std::list<ThreadTask> request_queue;

public:
    ThreadPool(int thread_s, int max_queue_s);

    ~ThreadPool();

    bool append(std::shared_ptr<void> arg, std::function<void(std::shared_ptr<void>)> fun);

    void shutdown(bool graceful);

private:
    static void *worker(void *args);

    void run();
};


