# A C++  Web Server

#### 介绍
一个轻量级的web服务器，目前支持GET、HEAD方法请求，采用的是：单进程+Reactor+非阻塞

#### 开发部署环境
· 操作系统: Ubuntu 16.04

· 编译器: g++ 9.3

· 版本控制: git

· 自动化构建: cmake

· 编辑器: Vscode

· 压测工具：WebBench


#### Usage

·  mkdir build && cd build
·  cmake .. && make
·  ./Socket [-p port] [-t thread_numbers]  [-r website_root_path] [-d daemon_run]

#### 核心功能及技术

1.  通过状态机解析HTTP请求，目前支持GET、HEAD方法
2.  添加定时器支持HTTP长连接，定时回调handler处理超时连接
3.  使用 priority queue 实现管理定时器，使用标记删除，以支持惰性删除，提高性能
4.  使用epoll + 非阻塞IO + 边缘触发(ET) 实现高并发处理请求，使用Reactor编程模型
5.  使用线程池提高并发度，并降低频繁创建线程的开销
6.  使用shared_ptr、weak_ptr管理指针，防止内存泄漏
7.  使用RAII手法封装互斥器(pthrea_mutex_t)、 条件变量(pthread_cond_t)等线程同步互斥机制，使用RAII管理文件描述符等资源
8.  epoll使用EPOLLONESHOT保证一个socket连接在任意时刻都只被一个线程处理
#### 开发计划

1.  添加异步的日志记录系统，记录服务器的运行和访问状态
2.  提供CGI支持
3.  类似nginx的反向代理和负载均衡
