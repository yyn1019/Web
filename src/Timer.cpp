
#include<sys/time.h>
#include<unistd.h>

#include"../include/Timer.h"
#include"../include/Epoll.h"

size_t TimerNode::current_msec=0;//当前时间
const size_t TimerManager::DEFAULT_TIME_OUT = 20 * 1000;//20s

TimerNode::TimerNode(std::shared_ptr<HttpData> httpData,size_t timeout):deleted_(false),httpData_(httpData){
    current_time();
    expiredTime_ = current_msec+timeout;
}

TimerNode::~TimerNode(){
    if(httpData_){
        auto it = Epoll::httpDataMap.find(httpData_->clientSocket_->fd);
        if(it != Epoll::httpDataMap.end()){
            Epoll::httpDataMap.erase(it);
        }
    }
}

void inline TimerNode::current_time(){
    struct timeval cur;
    gettimeofday(&cur, NULL);
    current_msec = (cur.tv_sec * 1000) + (cur.tv_usec / 1000);
}

void TimerNode::deleted(){

    httpData_.reset();
    deleted_ = true;
}
void TimerManager::addTimer(std::shared_ptr<HttpData> httpData, size_t timeout) {
    Shared_TimerNode timerNode(new TimerNode(httpData, timeout));
    {
        MutexGuard guard(lock_);
        TimerQueue.push(timerNode);
        //
        httpData->setTimer(timerNode);
    }
}

void TimerManager::handle_expired_event(){
    MutexGuard guard(lock_);

    //
    TimerNode::current_time();
    while (!TimerQueue.empty())
    {
        Shared_TimerNode timerNode = TimerQueue.top();
        if(timerNode->isDeleted()){
            TimerQueue.pop();
        }else if(timerNode->isExpire()){
            TimerQueue.pop();
        }else{
            break;
        }

    } 
}
