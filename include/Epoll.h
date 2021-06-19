#pragma once

#include<sys/epoll.h>
#include<iostream>
#include<vector>
#include<unordered_map>
#include<memory>

#include"HttpData.h"
#include"socket.h"
#include"Timer.h"

class Epoll
{
public:
    using Shared_HttpData = std::shared_ptr<HttpData>;
    static std::unordered_map<int,Shared_HttpData> httpDataMap;
    static const int MAX_EVENTS;
    static epoll_event* events;
    static TimerManager timerManager;
    const static __uint32_t DEFAULT_EVENTS;
public:
    static int init(int max_events);

    static int addfd(int epoll_fd,int fd,__uint32_t events,Shared_HttpData);

    static int modfd(int epoll_fd,int fd,__uint32_t events,Shared_HttpData);

    static int delfd(int epoll_fd,int fd,__uint32_t events);

    static std::vector<Shared_HttpData> 
    poll(const ServerSocket &serverSocket,int max_event,int timeout);

    static void handleConnection(const ServerSocket &serverSocket);
};
