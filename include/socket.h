#pragma once

#include<unistd.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/socket.h>

#include<iostream>
#include<memory>

class ClientSocket;

void setReusePort(int fd);

int setNonblocking(int fd);

class ServerSocket{

public:
    ServerSocket(int port=80,const char* ip=NULL);

    ~ServerSocket();

    void listen();

    void bind();

    void close();

    int accpet(ClientSocket & client)const;

public:
    sockaddr_in mAddr;
    int listen_fd;
    int epoll_fd;
    int mPort;
    const char* mIP;

};

class ClientSocket{

public:
    ClientSocket(){fd=-1;}

    void close();

    ~ClientSocket();

    int fd;
    sockaddr_in mAddr;
    socklen_t mLen;

};