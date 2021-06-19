#pragma once

#include"HttpData.h"
#include"MutexLock.h"

#include<queue>
#include<deque>
#include<memory>

class HttpData;

class TimerNode
{
private:
    bool deleted_;
    size_t expiredTime_; //毫秒
    std::shared_ptr<HttpData> httpData_;

public:
    TimerNode(std::shared_ptr<HttpData> httpData,size_t timeout);
    ~TimerNode();

public:
    bool isDeleted() const{return deleted_;}

    size_t getExpiredTime(){return expiredTime_;}

    bool isExpire(){
        return expiredTime_<current_msec;
    }

    void deleted();

    std::shared_ptr<HttpData> getHttpData(){return httpData_;}

    static void current_time();

    static size_t current_msec;//当前时间
};

struct TimerCmp
{
    bool operator()(std::shared_ptr<TimerNode> &a,std::shared_ptr<TimerNode> &b)const{
        return a->getExpiredTime() > b->getExpiredTime();
    }
};

class TimerManager{
public:
    using Shared_TimerNode = std::shared_ptr<TimerNode>;
    using Shared_HttpData = std::shared_ptr<HttpData>;

public:
    void addTimer(Shared_HttpData httpData,size_t timeout);

    void handle_expired_event();

    const static size_t DEFAULT_TIME_OUT;

private:
    std::priority_queue<Shared_TimerNode,std::deque<Shared_TimerNode>,TimerCmp> TimerQueue;
    MutexLock lock_;
};
