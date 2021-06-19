#include "../include/Epoll.h"
#include <iostream>
#include <vector>
#include <sys/epoll.h>
#include <cstdio>

std::unordered_map<int,std::shared_ptr<HttpData>> Epoll::httpDataMap;

const int Epoll::MAX_EVENTS = 1000;

epoll_event *Epoll::events;

//ET  |   可读 | 保证一个socket连接在任一时刻只被一个线程处理
const __uint32_t Epoll::DEFAULT_EVENTS = (EPOLLIN | EPOLLET | EPOLLONESHOT);

TimerManager Epoll::timerManager;

int Epoll::init(int max_events){
    int epoll_fd = ::epoll_create(max_events);
    if(epoll_fd == -1){
        std::cout<<"epoll create error"<<std::endl;
        exit(-1);
    }
    events = new epoll_event[max_events];
    return epoll_fd;
}

int Epoll::addfd(int epoll_fd,int fd,__uint32_t events,std::shared_ptr<HttpData> httpData){
    epoll_event event;
    event.events = (EPOLLIN | EPOLLET);
    event.data.fd = fd;
    //添加到httpdataMap
    httpDataMap[fd] = httpData;
    int ret = ::epoll_ctl(epoll_fd,EPOLL_CTL_ADD,fd,&event);
    if(ret < 0){
        std::cout<<"epoll add error"<<std::endl;
        //失败，释放httpData
        httpDataMap[fd].reset();
        return -1;
    }
    return 0;
}


int Epoll::modfd(int epoll_fd, int fd, __uint32_t events, std::shared_ptr<HttpData> httpData) {
    epoll_event event;
    event.events = events;
    event.data.fd = fd;
    // 每次更改的时候也更新 httpDataMap
    httpDataMap[fd] = httpData;
    int ret = ::epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event);
    if (ret < 0) {
        std::cout << "epoll mod error" << std::endl;
        // 释放httpData
        httpDataMap[fd].reset();
        return -1;
    }
    return 0;
}

int Epoll::delfd(int epoll_fd, int fd, __uint32_t events) {
    epoll_event event;
    event.events = events;
    event.data.fd = fd;
    int ret = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, &event);
    if (ret < 0) {
        std::cout << "epoll del error" << std::endl;
        return -1;
    }
    auto it = httpDataMap.find(fd);
    if (it != httpDataMap.end()) {
        httpDataMap.erase(it);
    }
    return 0;
}

void Epoll::handleConnection(const ServerSocket &serverSocket){

    std::shared_ptr<ClientSocket> tempClient(new ClientSocket);
    //ET模式，循环接收
    //listen_fd设置为non-blocking

    while (serverSocket.accpet(*tempClient) > 0)
    {
        int ret = setNonblocking(tempClient->fd);

        if(ret < 0){
            std::cout<<"setnonblocking error"<<std::endl;
            tempClient->close();
            continue;
        }

        // FIXME 接受新客户端 构造HttpData并添加定时器

        // 在这里做限制并发, 暂时未完成

        std::shared_ptr<HttpData> sharedHttpData(new HttpData);
        sharedHttpData->request_ = std::shared_ptr<HttpRequest>(new HttpRequest());
        sharedHttpData->response_ = std::shared_ptr<HttpResponse>(new HttpResponse());

        std::shared_ptr<ClientSocket> sharedClientSocket(new ClientSocket());
        sharedClientSocket.swap(tempClient);
        sharedHttpData->clientSocket_ = sharedClientSocket;
        sharedHttpData->epoll_fd = serverSocket.epoll_fd;

        addfd(serverSocket.epoll_fd,sharedClientSocket->fd,DEFAULT_EVENTS,sharedHttpData);
        timerManager.addTimer(sharedHttpData, TimerManager::DEFAULT_TIME_OUT);
    }

}


std::vector<std::shared_ptr<HttpData>> Epoll::poll(const ServerSocket &serverSocket, int max_events,int timeout){
    int event_num = epoll_wait(serverSocket.epoll_fd,events,max_events,timeout);
    if(event_num < 0){
        std::cout << "epoll_num" << event_num << std::endl;
        std::cout << "epoll_wait error" << std::endl;
        std::cout << errno << std::endl;
        exit(-1);
    }

    std::vector<std::shared_ptr<HttpData>> httpDatas;
    //遍历events集合
    for (int i = 0; i < event_num; i++)
    {
        int fd = events[i].data.fd;

        //监听描述符
        if(fd == serverSocket.listen_fd){
            handleConnection(serverSocket);
        }else{
            //出错的描述符，移除定时器，关闭文件描述符
            if((events[i].events & EPOLLERR) || (events[i].events & EPOLLRDHUP) || (events[i].events & EPOLLHUP)){
                auto it = httpDataMap.find(fd);
                if(it != httpDataMap.end()){
                    it->second->closeTimer();
                }
                continue;
            }

            auto it = httpDataMap.find(fd);
            if(it != httpDataMap.end()){
                if((events[i].events & EPOLLIN) || (events[i].events & EPOLLPRI)){
                    httpDatas.push_back(it->second);

                    it->second->closeTimer();
                    httpDataMap.erase(it);
                }
            }else{
                std::cout << "长链接第二次连接未找到 " << std::endl;
                ::close(fd);
                continue;
            }
        }
    }
    
    return httpDatas;
}