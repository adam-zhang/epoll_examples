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
#include <string>
#include <iostream>
#include <memory>
#include <functional>
#include <thread>
#include "EpollTcpClient.h"

// actually no need to implement a tcp client using epoll






// callback when packet received

// base class of EpollTcpServer, focus on Start(), Stop(), SendData(), RegisterOnRecvCallback()...





// the implementation of Epoll Tcp client







// stop epoll tcp client and release epoll

// connect to tcp server

// register a callback when packet received

// handle write events on fd (usually happens when sending big files)

// one loop per thread, call epoll_wait and handle all coming events





int main(int argc, char* argv[])
{
    std::string server_ip {"127.0.0.1"};
    uint16_t server_port { 6666 };
    if (argc >= 2)
    {
        server_ip = std::string(argv[1]);
    }
    if (argc >= 3)
    {
        server_port = std::atoi(argv[2]);
    }

    // create a tcp client
    auto tcp_client = std::make_shared<EpollTcpClient>(server_ip, server_port);
    if (!tcp_client)
    {
        std::cout << "tcp_client create faield!" << std::endl;
        exit(-1);
    }


    // recv callback in lambda mode, you can set your own callback here
    auto recv_call = [&](const PacketPtr& data) -> void
    {
        // just print recv data to stdout
        std::cout << "recv: " << data->message() << std::endl;
        return;
    };

    // register recv callback to epoll tcp client
    tcp_client->registerOnRecvCallback(recv_call);

    // start the epoll tcp client
    if (!tcp_client->start())
    {
        std::cout << "tcp_client start failed!" << std::endl;
        exit(1);
    }
    std::cout << "############tcp_client started!################" << std::endl;

    std::string message;
    while (true)
    {
        // read content from stdin
        std::cout << std::endl << "input:";
        std::getline(std::cin, message);
        auto packet = std::make_shared<Packet>(message);
        tcp_client->sendData(packet);
        //std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    tcp_client->stop();

    return 0;
}
