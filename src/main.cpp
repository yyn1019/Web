#include "../include/Server.h"
#include "../include/Util.h"

#include <string>
#include <iostream>
#include <dirent.h>
#include <stdio.h>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>  //for signal

std::string basePath = ".";   //默认是程序当前目录

void daemon_run(){
    int pid;
    signal(SIGCHLD, SIG_IGN);
    pid = fork();
    if(pid < 0){
        std::cout << "fork error" << std::endl;
        exit(0);
    }
    else if(pid > 0){
        exit(0);
    }

    setsid();
    int fd;
    fd = open("dev/null", O_RDWR,0);
    if(fd != -1){
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd,STDERR_FILENO);
    }
    if(fd > 2){
        close(fd);
    }
}

int main(int argc, char *argv[]){

    int threadNumber = 4;   //  默认线程数
    int port = 7244;        // 默认端口
    char tempPath[256];
    int opt;
    const char *str = "t:p:r:d";
    bool daemon = false;

    while ((opt = getopt(argc, argv, str))!= -1)
    {
        switch (opt)
        {
            case 't':
            {
                threadNumber = atoi(optarg);
                break;
            }
            case 'r':
            {
                int ret = check_base_path(optarg);
                if (ret == -1) {
                    printf("Warning: \"%s\" 不存在或不可访问, 将使用当前目录作为网站根目录\n", optarg);
                    if(getcwd(tempPath, 300) == NULL)
                    {
                        perror("getcwd error");
                        basePath = ".";
                    }
                    else
                    {
                        basePath = tempPath;
                    }
                    break;
                }
                if (optarg[strlen(optarg)-1] == '/') {
                    optarg[strlen(optarg)-1] = '\0';
                }
                basePath = optarg;
                break;
            }
            case 'p':
            {
                // FIXME 端口合法性校验
                port = atoi(optarg);
                break;
            }
            case 'd':
            {
                daemon = true;
                break;
            }

            default: break;
        }
    }

    if (daemon)
        daemon_run();


    //  输出配置信息
    {
      printf("*******LC WebServer 配置信息*******\n");
      printf("端口:\t%d\n", port);
      printf("线程数:\t%d\n", threadNumber);
      printf("根目录:\t%s\n", basePath.c_str());
    }
    handle_for_sigpipe();

    HttpServer httpServer(port);
    httpServer.run(threadNumber);
    return 0;
}