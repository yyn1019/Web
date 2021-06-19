

#include"../../include/HttpData.h"



void HttpData::closeTimer(){
    if(timer_.lock()){
        std::shared_ptr<TimerNode> tempTimer(timer_.lock());
        tempTimer->deleted();
        //断开weak_ptr
        timer_.reset();
    }
}

void HttpData::setTimer(std::shared_ptr<TimerNode> timer){
    timer_ = timer;
}