#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <cstring>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cassert>

#include <iostream>
#include <string>
#include <thread>
#include <memory>
#include <functional>

#include "EpollTcpServer.h"

// callback when packet received


int main(int argc, char* argv[])
{
    std::string local_ip {"127.0.0.1"};
    uint16_t local_port { 6666 };
    if (argc >= 2)
    {
        local_ip = std::string(argv[1]);
    }
    if (argc >= 3)
    {
        local_port = std::atoi(argv[2]);
    }
    // create a epoll tcp server
    auto epoll_server = std::make_shared<EpollTcpServer>(local_ip, local_port);
    if (!epoll_server)
    {
        std::cout << "tcp_server create faield!" << std::endl;
        exit(-1);
    }

    // recv callback in lambda mode, you can set your own callback here
    auto recv_call = [&](const PacketPtr& data) -> void
    {
        // just echo packet
        epoll_server->sendData(data);
        return;
    };

    // register recv callback to epoll tcp server
    epoll_server->registerOnRecvCallback(recv_call);

    // start the epoll tcp server
    if (!epoll_server->start())
    {
        std::cout << "tcp_server start failed!" << std::endl;
        exit(1);
    }
    std::cout << "############tcp_server started!################" << std::endl;

    // block here
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    epoll_server->stop();

    return 0;
}


