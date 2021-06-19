#pragma once

#include "HttpParse.h"
#include "HttpResponse.h"
#include"HttpRequest.h"
#include "socket.h"
#include "Timer.h"
#include <memory>

class TimerNode;

class HttpData : public std::enable_shared_from_this<HttpData> {
private:
    std::weak_ptr<TimerNode> timer_;
public:
    int epoll_fd;

public:
    HttpData():epoll_fd(-1){}

public:
    std::shared_ptr<HttpRequest> request_;
    std::shared_ptr<HttpResponse> response_;
    std::shared_ptr<ClientSocket> clientSocket_;

    void closeTimer();

    void setTimer(std::shared_ptr<TimerNode>);
};
