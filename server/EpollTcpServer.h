/********************************************************************************
> FileName:	EpollTcpServer.h
> Author:	Mingping Zhang
> Email:	mingpingzhang@163.com
> Create Time:	Mon Mar 13 09:50:49 2023
********************************************************************************/
#ifndef EPOLLTCPSERVER_H
#define EPOLLTCPSERVER_H

#include "EpollTcpBase.h"
#include <thread>

class EpollTcpServer : public EpollTcpBase
{
public:
    EpollTcpServer()                                       = default;
    EpollTcpServer(const EpollTcpServer& other)            = delete;
    EpollTcpServer& operator=(const EpollTcpServer& other) = delete;
    EpollTcpServer(EpollTcpServer&& other)                 = delete;
    EpollTcpServer& operator=(EpollTcpServer&& other)      = delete;
    ~EpollTcpServer() override;

    // the local ip and port of tcp server
    EpollTcpServer(const std::string& local_ip, uint16_t local_port);

public:
    // start tcp server
    bool start() override;
    // stop tcp server
    bool stop() override;
    // send packet
    int32_t sendData(const PacketPtr& data) override;
    // register a callback when packet received
    void registerOnRecvCallback(callback_recv_t callback) override;
    void unregisterOnRecvCallback() override;

protected:
    // create epoll instance using epoll_create and return a fd of epoll
    int32_t createEpoll();
    // create a socket fd using api socket()
    int32_t createSocket();
    // set socket noblock
    int32_t makeSocketNonBlock(int32_t fd);
    // listen()
    int32_t listen(int32_t listenfd);
    // add/modify/remove a item(socket/fd) in epoll instance(rbtree), for this example, just add a socket to epoll rbtree
    int32_t updateEpollEvents(int efd, int op, int fd, int events);

    // handle tcp accept event
    void onSocketAccept();
    // handle tcp socket readable event(read())
    void onSocketRead(int32_t fd);
    // handle tcp socket writeable event(write())

    void onSocketWrite(int32_t fd);
    // one loop per thread, call epoll_wait and return ready socket(accept,readable,writeable,error...)
    void epollLoop();


private:
    std::string localIP_; // tcp local ip
    uint16_t localPort_ = 0; // tcp bind local port
    int32_t handle_ = -1 ; // listenfd
    int32_t efd_ = -1 ; // epoll fd
    std::shared_ptr<std::thread> th_loop_ { nullptr }; // one loop per thread(call epoll_wait in loop)
    bool loopFlag_ = true ; // if loop_flag_ is false, then exit the epoll loop
    callback_recv_t recvCallback_ = nullptr ; // callback when received
};

#endif//EPOLLTCPSERVER_H
