/********************************************************************************
> FileName:	EpollTcpClient.h
> Author:	Mingping Zhang
> Email:	mingpingzhang@163.com
> Create Time:	Mon Mar 13 10:14:11 2023
********************************************************************************/
#ifndef EPOLLTCPCLIENT_H
#define EPOLLTCPCLIENT_H

#include "EpollTcpBase.h"
#include <thread>

class EpollTcpClient : public EpollTcpBase
{
public:
    EpollTcpClient()                                       = default;
    EpollTcpClient(const EpollTcpClient& other)            = delete;
    EpollTcpClient& operator=(const EpollTcpClient& other) = delete;
    EpollTcpClient(EpollTcpClient&& other)                 = delete;
    EpollTcpClient& operator=(EpollTcpClient&& other)      = delete;
    ~EpollTcpClient() override;

    // the server ip and port
    EpollTcpClient(const std::string& server_ip, uint16_t server_port);

public:
    bool start() override;
    bool stop() override;
    int32_t sendData(const PacketPtr& data) override;
    void registerOnRecvCallback(callback_recv_t callback) override;
    void unregisterOnRecvCallback() override;

protected:
    // create epoll instance using epoll_create and return a fd of epoll
    int32_t createEpoll();
    // create a socket fd using api socket()
    int32_t createSocket();
    // connect to server
    int32_t connect(int32_t listenfd);
    // add/modify/remove a item(socket/fd) in epoll instance(rbtree), for this example, just add a socket to epoll rbtree
    int32_t updateEpollEvents(int efd, int op, int fd, int events);
    // handle tcp socket readable event(read())
    void onSocketRead(int32_t fd);
    // handle tcp socket writeable event(write())
    void onSocketWrite(int32_t fd);
    // one loop per thread, call epoll_wait and return ready socket(readable,writeable,error...)
    void epollLoop();


private:
    std::string server_ip_; // tcp server ip
    uint16_t server_port_ { 0 }; // tcp server port
    int32_t handle_ { -1 }; // client fd
    int32_t efd_ { -1 }; // epoll fd
    std::shared_ptr<std::thread> th_loop_ { nullptr }; // one loop per thread(call epoll_wait in loop)
    bool loop_flag_ { true }; // if loop_flag_ is false, then exit the epoll loop
    callback_recv_t recv_callback_ { nullptr }; // callback when received
};


#endif//EPOLLTCPCLIENT_H
