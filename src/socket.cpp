#include"../include/socket.h"
#include<sys/types.h>
#include<fcntl.h>

void setReusePort(int fd){
    int opt=1;
    setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,(const void*)&opt,sizeof(opt));
}
int setNonblocking(int fd){
    int old_option=fcntl(fd,F_GETFL);
    int new_option=old_option | O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
    return old_option;
}

ServerSocket::ServerSocket(int port,const char* ip):mPort(port),mIP(ip){
    mAddr.sin_family=AF_INET;
    mAddr.sin_port=htons(port);
    if(ip!=nullptr){
        ::inet_pton(AF_INET,ip,&mAddr.sin_addr);
    }
    else{
        mAddr.sin_addr.s_addr=htonl(INADDR_ANY);
    }
    listen_fd=socket(AF_INET,SOCK_STREAM,0);
    if(listen_fd==-1){
        std::cout<<"create socket error in file "<<__FILE__<<"at: "<<__LINE__<<std::endl;
    }
    setReusePort(listen_fd);
    setNonblocking(listen_fd);
}

ServerSocket::~ServerSocket(){
    close();
}

void ServerSocket::listen(){
    int ret = ::listen(listen_fd,1024);
    if(ret==-1){
        std::cout << "listen error in file <" << __FILE__ << "> "<< "at " << __LINE__ << std::endl;
        exit(0);
    }
       
}

void ServerSocket::bind(){
    int ret = ::bind(listen_fd,(struct sockaddr*)&mAddr,sizeof(mAddr));
    if(ret==-1){
        std::cout << "bind error in file <" << __FILE__ << "> "<< "at " << __LINE__ << std::endl;
        exit(0);
    }
}

void ServerSocket::close(){
    if(listen_fd>=0){
        ::close(listen_fd);
        listen_fd=-1;
    }
}

int ServerSocket::accpet(ClientSocket & client)const{

    int clientfd=::accept(listen_fd,NULL,NULL);

    if(clientfd<0){
        if((errno==EWOULDBLOCK)||(errno==EAGAIN))
            return clientfd;
        std::cout << "accept error in file <" << __FILE__ << "> "<< "at " << __LINE__ << std::endl;
        std::cout<<"clientfd : "<<clientfd<<std::endl;
        perror("accept error: ");

    }

    client.fd=clientfd;
    return clientfd;
}

void ClientSocket::close(){
    if(fd>=0){
        ::close(fd);
        fd=-1;
    }
}

ClientSocket::~ClientSocket(){
    close();
}