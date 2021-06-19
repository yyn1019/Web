#pragma once
#include<pthread.h>
#include"noncopyable.h"


class MutexLock:public noncopyable
{
private:
    pthread_mutex_t mutex;
public:
    
    MutexLock(){
        pthread_mutex_init(&mutex,NULL);
    }
    ~MutexLock(){
        pthread_mutex_destroy(&mutex);
    }
    void lock(){
        pthread_mutex_lock(&mutex);
    }
    void unlock(){
        pthread_mutex_unlock(&mutex);
    }
    pthread_mutex_t* getMutex(){
        return &mutex;
    }
};

class MutexGuard:public noncopyable{
public:
    explicit MutexGuard(MutexLock &mutex):mutex_(mutex){
        mutex_.lock();
    }
    ~MutexGuard(){
        mutex_.unlock();
    }
private:
    MutexLock &mutex_;
};