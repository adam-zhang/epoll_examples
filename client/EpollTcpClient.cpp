/********************************************************************************
  > FileName:	EpollTcpClient.cpp
  > Author:	Mingping Zhang
  > Email:	mingpingzhang@163.com
  > Create Time:	Mon Mar 13 10:14:11 2023
 ********************************************************************************/

#include "EpollTcpClient.h"
#include <iostream>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cassert>

EpollTcpClient::EpollTcpClient(const std::string& server_ip, uint16_t server_port)
    : server_ip_ ( server_ip ),
      server_port_ ( server_port )
{
}

EpollTcpClient::~EpollTcpClient()
{
    stop();
}

bool EpollTcpClient::start()
{
    // create epoll instance
    if (createEpoll() < 0)
    {
        return false;
    }
    // create socket and bind
    int cli_fd  = createSocket();
    if (cli_fd < 0)
    {
        return false;
    }

    // connect to server
    int lr = this->connect(cli_fd);
    if (lr < 0)
    {
        return false;
    }
    std::cout << "EpollTcpClient Init success!" << std::endl;
    handle_ = cli_fd;

    // after connected successfully, add this socket to epoll instance, and focus on EPOLLIN and EPOLLOUT event
    int er = updateEpollEvents(efd_, EPOLL_CTL_ADD, handle_, EPOLLIN | EPOLLET);
    if (er < 0)
    {
        // if something goes wrong, close listen socket and return false
        ::close(handle_);
        return false;
    }

    assert(!th_loop_);

    // the implementation of one loop per thread: create a thread to loop epoll
    th_loop_ = std::make_shared<std::thread>(&EpollTcpClient::epollLoop, this);
    if (!th_loop_)
    {
        return false;
    }
    // detach the thread(using loop_flag_ to control the start/stop of loop)
    th_loop_->detach();

    return true;
}

bool EpollTcpClient::stop()
{
    loop_flag_ = false;
    ::close(handle_);
    ::close(efd_);
    std::cout << "stop epoll!" << std::endl;
    unregisterOnRecvCallback();
    return true;
}

int32_t EpollTcpClient::createEpoll()
{
    // the basic epoll api of create a epoll instance
    int epollfd = epoll_create(1);
    if (epollfd < 0)
    {
        // if something goes wrong, return -1
        std::cout << "epoll_create failed!" << std::endl;
        return -1;
    }
    efd_ = epollfd;
    return epollfd;
}

int32_t EpollTcpClient::createSocket()
{
    // create tcp socket
    int s = ::socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0)
    {
        std::cout << "create socket failed!" << std::endl;
        return -1;
    }

    return s;
}

int32_t EpollTcpClient::connect(int32_t cli_fd)
{
    struct sockaddr_in address = {0};  // server info
    address.sin_family = AF_INET;
    address.sin_port = htons(server_port_);
    address.sin_addr.s_addr  = inet_addr(server_ip_.c_str());

    int r = ::connect(cli_fd, (struct sockaddr*)&address, sizeof(address));
    if ( r < 0)
    {
        std::cout << "connect failed! r=" << r << " errno:" << errno << std::endl;
        return -1;
    }
    return 0;
}

// add/modify/remove a item(socket/fd) in epoll instance(rbtree), for this example, just add a socket to epoll rbtree
int32_t EpollTcpClient::updateEpollEvents(int efd, int op, int fd, int events)
{
    struct epoll_event ev = {0};
    ev.events = events;
    ev.data.fd = fd;
    fprintf(stdout,"%s fd %d events read %d write %d\n", op == EPOLL_CTL_MOD ? "mod" : "add", fd, ev.events & EPOLLIN, ev.events & EPOLLOUT);
    int r = epoll_ctl(efd, op, fd, &ev);
    if (r < 0)
    {
        std::cout << "epoll_ctl failed!" << std::endl;
        return -1;
    }
    return 0;
}


void EpollTcpClient::registerOnRecvCallback(callback_recv_t callback)
{
    assert(!recv_callback_);
    recv_callback_ = callback;
}

void EpollTcpClient::unregisterOnRecvCallback()
{
    assert(recv_callback_);
    recv_callback_ = nullptr;
}

// handle read events on fd
void EpollTcpClient::onSocketRead(int32_t fd)
{
    char read_buf[4096] = {0};
    int n = -1;
    while ( (n = ::read(fd, read_buf, sizeof(read_buf))) > 0)
    {
        // callback for recv
        std::string msg(read_buf, n);
        PacketPtr data = std::make_shared<Packet>(fd, msg);
        if (recv_callback_)
        {
            // handle recv packet
            recv_callback_(data);
        }
    }
    if (n == -1)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // read finished
            return;
        }
        // something goes wrong for this fd, should close it
        ::close(fd);
        return;
    }
    if (n == 0)
    {
        // this may happen when client close socket. EPOLLRDHUP usually handle this, but just make sure; should close this fd
        ::close(fd);
        return;
    }
}


void EpollTcpClient::onSocketWrite(int32_t fd)
{
    std::cout << "fd: " << fd << " writeable!" << std::endl;
}

int32_t EpollTcpClient::sendData(const PacketPtr& data)
{
    int r = ::write(handle_, data->message().data(), data->message().size());
    if (r == -1)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return -1;
        }
        // error happend
        ::close(handle_);
        std::cout << "fd: " << handle_ << " write error, close it!" << std::endl;
        return -1;
    }
    return r;
}


void EpollTcpClient::epollLoop()
{
    // request some memory, if events ready, socket events will copy to this memory from kernel
    struct epoll_event* alive_events =  static_cast<epoll_event*>(calloc(kMaxEvents, sizeof(epoll_event)));
    if (!alive_events)
    {
        std::cout << "calloc memory failed for epoll_events!" << std::endl;
        return;
    }
    while (loop_flag_)
    {
        int num = epoll_wait(efd_, alive_events, kMaxEvents, kEpollWaitTime);

        for (int i = 0; i < num; ++i)
        {
            int fd = alive_events[i].data.fd;
            int events = alive_events[i].events;

            if ( (events & EPOLLERR) || (events & EPOLLHUP) )
            {
                std::cout << "epoll_wait error!" << std::endl;
                // An error has occured on this fd, or the socket is not ready for reading (why were we notified then?).
                ::close(fd);
            }
            else  if (events & EPOLLRDHUP)
            {
                // Stream socket peer closed connection, or shut down writing half of connection.
                // more inportant, We still to handle disconnection when read()/recv() return 0 or -1 just to be sure.
                std::cout << "fd:" << fd << " closed EPOLLRDHUP!" << std::endl;
                // close fd and epoll will remove it
                ::close(fd);
            }
            else if ( events & EPOLLIN )
            {
                // other fd read event coming, meaning data coming
                onSocketRead(fd);
            }
            else if ( events & EPOLLOUT )
            {
                // write event for fd (not including listen-fd), meaning send buffer is available for big files
                onSocketWrite(fd);
            }
            else
            {
                std::cout << "unknow epoll event!" << std::endl;
            }
        } // end for (int i = 0; ...

    } // end while (loop_flag_)
    free(alive_events);
}
