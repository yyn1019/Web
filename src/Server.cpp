

#include "../include/Server.h"
#include "../include/HttpParse.h"
#include "../include/HttpResponse.h"
#include "../include/ThreadPool.h"
#include "../include/HttpData.h"
#include "../include/Epoll.h"

#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <iostream>
#include <string>
#include <functional>
#include <sys/epoll.h>
#include <vector>
#include <cstring>


char NOT_FOUND_PAGE[] = "<html>\n"
                        "<head><title>404 Not Found</title></head>\n"
                        "<body bgcolor=\"white\">\n"
                        "<center><h1>404 Not Found</h1></center>\n"
                        "<hr><center>LC WebServer/0.3 (Linux)</center>\n"
                        "</body>\n"
                        "</html>";

char FORBIDDEN_PAGE[] = "<html>\n"
                        "<head><title>403 Forbidden</title></head>\n"
                        "<body bgcolor=\"white\">\n"
                        "<center><h1>403 Forbidden</h1></center>\n"
                        "<hr><center>LC WebServer/0.3 (Linux)</center>\n"
                        "</body>\n"
                        "</html>";

char INDEX_PAGE[] = "<!DOCTYPE html>\n"
                    "<html>\n"
                    "<head>\n"
                    "    <title>Welcome to LC WebServer!</title>\n"
                    "    <style>\n"
                    "        body {\n"
                    "            width: 35em;\n"
                    "            margin: 0 auto;\n"
                    "            font-family: Tahoma, Verdana, Arial, sans-serif;\n"
                    "        }\n"
                    "    </style>\n"
                    "</head>\n"
                    "<body>\n"
                    "<h1>Welcome to Love_z_s WebServer!</h1>\n"
                    "<p>If you see this page, the lc webserver is successfully installed and\n"
                    "    working. </p>\n"
                    "<p><em>Thank you for using Love_z_s WebServer.</em></p>\n"
                    "</body>\n"
                    "</html>";


char TEST[] = "HELLO WORLD";

extern std::string basePath;

const char *default_page = "../src/sceen/1.html";

void HttpServer::run(int thread_num, int max_queue_size){
    ThreadPool threadPool(thread_num, max_queue_size);

    int epoll_fd = Epoll::init(1024);

    std::shared_ptr<HttpData> httpData(new HttpData());
    httpData->epoll_fd = epoll_fd;
    serverSocket.epoll_fd = epoll_fd;

    __uint32_t event = (EPOLLIN | EPOLLET);
    Epoll::addfd(epoll_fd,serverSocket.listen_fd,event,httpData);

    while (true)
    {
        std::vector<std::shared_ptr<HttpData>> events = Epoll::poll(serverSocket, 1024,-1);

        for(auto& req : events){
            threadPool.append(req,std::bind(&HttpServer::do_request, this, std::placeholders::_1));
        }
        //处理定时器超时事件
        Epoll::timerManager.handle_expired_event();
    }
    
}


void HttpServer::do_request(std::shared_ptr<void> arg){
    std::shared_ptr<HttpData> sharedHttpData = std::static_pointer_cast<HttpData>(arg);

    char buffer[BUFFERSIZE];

    bzero(buffer,BUFFERSIZE);
    int check_index = 0, read_index = 0,start_line = 0;
    ssize_t recv_data;
    HttpRequestParser::PARSE_STATE parse_state = HttpRequestParser::PARSE_REQUESTLINE;

    while (true)
    {
        recv_data = recv(sharedHttpData->clientSocket_->fd, buffer + read_index, BUFFERSIZE - read_index, 0);
        if(recv_data == -1){
            if((errno == EAGAIN) || (errno == EWOULDBLOCK)){
                return;
            }
            std::cout<<"reading faild" << std::endl;
            return;
        }
        //返回值为0对端关闭，也应该关闭定时器
        if(recv_data == 0){
            std::cout << "connection closed by peer" << std::endl;
            break;
        }
        read_index += recv_data;

        HttpRequestParser::HTTP_CODE retcode = HttpRequestParser::parse_content(
                    buffer, check_index, read_index, parse_state,start_line, *sharedHttpData->request_);
        
        if(retcode == HttpRequestParser::NO_REQUEST){
            continue;
        }

        if(retcode == HttpRequestParser::GET_REQUEST){

            auto it = sharedHttpData->request_->mHeaders.find(HttpRequest::Connection);
            if(it != sharedHttpData->request_->mHeaders.end()){
                if(it->second == "keep-alive"){
                    sharedHttpData->response_->setKeepAlive(true);
                    //timeout = 20s
                    sharedHttpData->response_->addHeader("Keep-Alive",std::string("timeout=20"));
                }else{
                    sharedHttpData->response_->setKeepAlive(false);
                }
            }

            header(sharedHttpData);
            getMime(sharedHttpData);

            FileState fileState = static_file(sharedHttpData, basePath.c_str());
            send(sharedHttpData, fileState);

            if(sharedHttpData->response_->keep_alive()){
                Epoll::modfd(sharedHttpData->epoll_fd, sharedHttpData->clientSocket_->fd, Epoll::DEFAULT_EVENTS,sharedHttpData);
                Epoll::timerManager.addTimer(sharedHttpData,TimerManager::DEFAULT_TIME_OUT);
            }
        }else{
            std::cout << "Bad Request" <<std::endl;
        }
    }
    
}


void HttpServer::header(std::shared_ptr<HttpData> httpData){
    if(httpData->request_->mVersion == HttpRequest::HTTP_11){
        httpData->response_->setVersion(HttpRequest::HTTP_11);
    }else{
        httpData->response_->setVersion((HttpRequest::HTTP_10));
    }
    httpData->response_->addHeader("Server","LOVE WebServer");
}

//获取Mime同时设置path到response
void HttpServer::getMime(std::shared_ptr<HttpData> httpData){
    std::string filepath = httpData->request_->mUri;
    std::string mime;
    int pos;
    //这里直接将参数丢掉了，后续。。。
    if((pos = filepath.rfind('?')) != std::string::npos){
        filepath.erase(filepath.rfind('?'));
    }

    if(filepath.rfind('.') != std::string::npos){
        mime = filepath.substr(filepath.rfind('.'));
    }

    decltype(Mime_map)::iterator it;

    if((it = Mime_map.find(mime)) != Mime_map.end()){
        httpData->response_->setMime(it->second);
    }else{
        httpData->response_->setMime(Mime_map.find("default")->second);
    }
    httpData->response_->setFilePath(filepath);

}

HttpServer::FileState HttpServer::static_file(std::shared_ptr<HttpData> httpData, const char *basepath){
    struct stat file_stat;
    char file[strlen(basepath) + strlen(httpData->response_->filePath().c_str())+1];

    strcpy(file, basepath);
    strcat(file, httpData->response_->filePath().c_str());

    //文件不存在
    if(httpData->response_->filePath() == "/" || stat(file, &file_stat) < 0){
        httpData->response_->setMime(MimeType("text/html"));
        if(httpData->response_->filePath() =="/"){
            httpData->response_->setStatusCode(HttpResponse::k2000k);
            httpData->response_->setStatusMsg("OK");
        }else{
            httpData->response_->setStatusCode(HttpResponse::k404NotFound);
            httpData->response_->setStatusMsg("Not Found");
        }
        return FILE_NOT_FOUND;
    }
    //无访问权限
    if(!S_ISREG(file_stat.st_mode)){
        //Mime = html
        httpData->response_->setMime(MimeType("text/html"));
        httpData->response_->setStatusCode(HttpResponse::k403forbiden);
        httpData->response_->setStatusMsg("ForBidden");

        std::cout << "not normal file" << std::endl;
        return FILE_FORBIDDEN;
    }

    httpData->response_->setStatusCode(HttpResponse::k2000k);
    httpData->response_->setStatusMsg("OK");
    httpData->response_->setFilePath(file);

    return FILE_OK;
}

void HttpServer::send(std::shared_ptr<HttpData> httpData, FileState fileState){
    char header[BUFFERSIZE];
    bzero(header, '\0');
    const char *internal_error = "Internal Error";
    struct stat file_stat;
    httpData->response_->appenBuffer(header);
    //404
    if(fileState == FILE_NOT_FOUND){

        //"/"开头为默认页面
        if(httpData->response_->filePath() == std::string("/")){
            sprintf(strchr(header,'\0'), "Content-length: %lu\r\n\r\n",strlen(INDEX_PAGE));
            sprintf(strchr(header,'\0'),"%s", INDEX_PAGE);
        }else{
            sprintf(strchr(header,'\0'), "Content-length: %lu\r\n\r\n",strlen(NOT_FOUND_PAGE));
            sprintf(strchr(header,'\0'),"%s", NOT_FOUND_PAGE);
        }
        ::send(httpData->clientSocket_->fd,header, strlen(header), 0);
        /*if (httpData->response_->filePath() == std::string("/")) {
            FILE* file = fopen(default_page, "rb");
            if (file == NULL) {
                std::cout << "file open error " << std::endl;
                return;
            }
            fseek(file, 0, SEEK_END);
            int size = ftell(file);
            fseek(file, 0, SEEK_SET);
            int readLen,sendlen;
            char* fileBuf=(char*)malloc(sizeof(char)*size);   
            sprintf(strchr(header,'\0'), "Accept-Ranges: bytes\r\nContent-length: %lu\r\n\r\n", size);
            ::send(httpData->clientSocket_->fd, header, strlen(header), 0);
            do
            {
                readLen = fread(fileBuf, sizeof(char), size, file);
                if (readLen > 0) {
                    sendlen = ::send(httpData->clientSocket_->fd, fileBuf, readLen, 0);
                    size -= readLen;
                }

            } while (size > 0 && readLen > 0);
            free(fileBuf);
            fclose(file);            
        }else {
            sprintf(header, "%sContent-length: %d\r\n\r\n", header, strlen(NOT_FOUND_PAGE));
            sprintf(header, "%s%s", header, NOT_FOUND_PAGE);
            ::send(httpData->clientSocket_->fd, header, strlen(header), 0);
        }*/
        
        return;
    }

    if (fileState == FILE_FORBIDDEN) {
        sprintf(strchr(header,'\0'), "Content-length: %lu\r\n\r\n", strlen(FORBIDDEN_PAGE));
        sprintf(strchr(header,'\0'), "%s", FORBIDDEN_PAGE);
        ::send(httpData->clientSocket_->fd, header, strlen(header), 0);
        return;
    }
    // 获取文件状态
    if (stat(httpData->response_->filePath().c_str(), &file_stat) < 0) {
        sprintf(strchr(header,'\0'), "Content-length: %lu\r\n\r\n", strlen(internal_error));
        sprintf(strchr(header,'\0'), "%s", internal_error);
        ::send(httpData->clientSocket_->fd, header, strlen(header), 0);
        return;
    }

    int filefd = ::open(httpData->response_->filePath().c_str(), O_RDONLY);
    // 内部错误
    if (filefd < 0) {
        std::cout << "打开文件失败" << std::endl;
        sprintf(strchr(header,'\0'), "Content-length: %ld\r\n\r\n",strlen(internal_error));
        sprintf(strchr(header,'\0'), "%s", internal_error);
        ::send(httpData->clientSocket_->fd, header, strlen(header), 0);
        close(filefd);
        return;
    }

    sprintf(strchr(header,'\0'),"Content-length: %lu\r\n\r\n", file_stat.st_size);
    ::send(httpData->clientSocket_->fd, header, strlen(header), 0);
    void *mapbuf = mmap(NULL, file_stat.st_size, PROT_READ, MAP_PRIVATE, filefd, 0);
    ::send(httpData->clientSocket_->fd, mapbuf, file_stat.st_size, 0);
    munmap(mapbuf, file_stat.st_size);
    close(filefd);
    return;
err:
    sprintf(strchr(header,'\0'), "Content-length: %lu\r\n\r\n", strlen(internal_error));
    sprintf(strchr(header,'\0'), "%s", internal_error);
    ::send(httpData->clientSocket_->fd, header, strlen(header), 0);
    return;
}