/********************************************************************************
  > FileName:	EpollTcpServer.cpp
  > Author:	Mingping Zhang
  > Email:	mingpingzhang@163.com
  > Create Time:	Mon Mar 13 09:50:49 2023
 ********************************************************************************/

#include "EpollTcpServer.h"
#include "AppDef.h"
#include <iostream>
#include <cassert>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <vector>


EpollTcpServer::EpollTcpServer(const std::string& local_ip, uint16_t local_port)
	: localIP_ ( local_ip ),
	localPort_ ( local_port )
{
}

EpollTcpServer::~EpollTcpServer()
{
	stop();
}


bool EpollTcpServer::start()
{
	// create epoll instance
	if (createEpoll() < 0)
	{
		return false;
	}
	// create socket and bind
	int listenfd = createSocket();
	if (listenfd < 0)
	{
		return false;
	}
	// set listen socket noblock
	int mr = makeSocketNonBlock(listenfd);
	if (mr < 0)
	{
		return false;
	}

	// call listen()
	int lr = listen(listenfd);
	if (lr < 0)
	{
		return false;
	}
	std::cout << "EpollTcpServer Init success!" << std::endl;
	handle_ = listenfd;

	// add listen socket to epoll instance, and focus on event EPOLLIN and EPOLLOUT, actually EPOLLIN is enough
	int er = updateEpollEvents(efd_, EPOLL_CTL_ADD, handle_, EPOLLIN | EPOLLET);
	if (er < 0)
	{
		// if something goes wrong, close listen socket and return false
		::close(handle_);
		return false;
	}

	assert(!th_loop_);

	// the implementation of one loop per thread: create a thread to loop epoll
	th_loop_ = std::make_shared<std::thread>(&EpollTcpServer::epollLoop, this);
	if (!th_loop_)
	{
		return false;
	}
	// detach the thread(using loop_flag_ to control the start/stop of loop)
	th_loop_->detach();

	return true;
}

bool EpollTcpServer::stop()
{
	// set loop_flag_ false to stop epoll loop
	loopFlag_ = false;
	::close(handle_);
	::close(efd_);
	std::cout << "stop epoll!" << std::endl;
	unregisterOnRecvCallback();
	return true;
}

int32_t EpollTcpServer::createEpoll()
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


int32_t EpollTcpServer::createSocket()
{
	// create tcp socket
	int listenfd = ::socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd < 0)
	{
		std::cout << "create socket " << localIP_ << ":" << localPort_ << " failed!" << std::endl;
		return -1;
	}

	struct sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(localPort_);
	addr.sin_addr.s_addr  = inet_addr(localIP_.c_str());

	// bind to local ip and local port
	int r = ::bind(listenfd, (struct sockaddr*)&addr, sizeof(struct sockaddr));
	if (r != 0)
	{
		std::cout << "bind socket " << localIP_ << ":" << localPort_ << " failed!" << std::endl;
		::close(listenfd);
		return -1;
	}
	std::cout << "create and bind socket " << localIP_ << ":" << localPort_ << " success!" << std::endl;
	return listenfd;
}


int32_t EpollTcpServer::makeSocketNonBlock(int32_t fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0)
	{
		std::cout << "fcntl failed!" << std::endl;
		return -1;
	}
	int r = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	if (r < 0)
	{
		std::cout << "fcntl failed!" << std::endl;
		return -1;
	}
	return 0;
}


int32_t EpollTcpServer::listen(int32_t listenfd)
{
	int r = ::listen(listenfd, SOMAXCONN);
	if ( r < 0)
	{
		std::cout << "listen failed!" << std::endl;
		return -1;
	}
	return 0;
}

// add/modify/remove a item(socket/fd) in epoll instance(rbtree), for this example, just add a socket to epoll rbtree
int32_t EpollTcpServer::updateEpollEvents(int efd, int op, int fd, int events)
{
	struct epoll_event ev = {0};
	ev.events = events;
	ev.data.fd = fd; // ev.data is a enum
	fprintf(stdout,"%s fd %d events read %d write %d\n", op == EPOLL_CTL_MOD ? "mod" : "add", fd, ev.events & EPOLLIN, ev.events & EPOLLOUT);
	int r = epoll_ctl(efd, op, fd, &ev);
	if (r < 0)
	{
		std::cout << "epoll_ctl failed!" << std::endl;
		return -1;
	}
	return 0;
}


void EpollTcpServer::onSocketAccept()
{
	// epoll working on et mode, must read all coming data, so use a while loop here
	while (true)
	{
		struct sockaddr_in in_addr;
		socklen_t in_len = sizeof(in_addr);

		// accept a new connection and get a new socket
		int cli_fd = accept(handle_, (struct sockaddr*)&in_addr, &in_len);
		if (cli_fd == -1)
		{
			if ( (errno == EAGAIN) || (errno == EWOULDBLOCK) )
			{
				// read all accept finished(epoll et mode only trigger one time,so must read all data in listen socket)
				std::cout << "accept all coming connections!" << std::endl;
				break;
			}
			else
			{
				std::cout << "accept error!" << std::endl;
				continue;
			}
		}

		sockaddr_in peer;
		socklen_t p_len = sizeof(peer);
		// get client ip and port
		int r = getpeername(cli_fd, (struct sockaddr*)&peer, &p_len);
		if (r < 0)
		{
			std::cout << "getpeername error!" << std::endl;
			continue;
		}
		std::cout << "accpet connection from " << inet_ntoa(in_addr.sin_addr) << std::endl;
		int mr = makeSocketNonBlock(cli_fd);
		if (mr < 0)
		{
			::close(cli_fd);
			continue;
		}

		//  add this new socket to epoll instance, and focus on EPOLLIN and EPOLLOUT and EPOLLRDHUP event
		int er = updateEpollEvents(efd_, EPOLL_CTL_ADD, cli_fd, EPOLLIN | EPOLLRDHUP | EPOLLET);
		if (er < 0 )
		{
			// if something goes wrong, close this new socket
			::close(cli_fd);
			continue;
		}
	}
}


void EpollTcpServer::registerOnRecvCallback(callback_recv_t callback)
{
	assert(!recvCallback_);
	recvCallback_ = callback;
}

void EpollTcpServer::unregisterOnRecvCallback()
{
	assert(recvCallback_);
	recvCallback_ = nullptr;
}


void EpollTcpServer::onSocketRead(int32_t fd)
{
	char buffer[4096] = {0};
	int n = -1;
	// epoll working on et mode, must read all data
	while ( (n = ::read(fd, buffer, sizeof(buffer))) > 0)
	{
		// callback for recv
		std::cout << "fd: " << fd <<  " recv: " << buffer << std::endl;
		std::string message(buffer, n);
		// create a recv packet
		PacketPtr data = std::make_shared<Packet>(fd, message);
		if (recvCallback_)
		{
			// handle recv packet
			recvCallback_(data);
		}
	}
	if (n == -1)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
			// read all data finished
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

void EpollTcpServer::onSocketWrite(int32_t fd)
{
	// TODO(smaugx) not care for now
	std::cout << "fd: " << fd << " writeable!" << std::endl;
}

// send packet
int32_t EpollTcpServer::sendData(const PacketPtr& data)
{
	if (data->fd() == -1)
	{
		return -1;
	}
	// send packet on fd
	int r = ::write(data->fd(), data->message().data(), data->message().size());
	if (r == -1)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
			return -1;
		}
		// error happend
		::close(data->fd());
		std::cout << "fd: " << data->fd() << " write error, close it!" << std::endl;
		return -1;
	}
	std::cout << "fd: " << data->fd() << " write size: " << r << " ok!" << std::endl;
	return r;
}


void EpollTcpServer::epollLoop()
{
	// request some memory, if events ready, socket events will copy to this memory from kernel
	struct epoll_event* alive_events =  static_cast<epoll_event*>(calloc(MaxEvents(), sizeof(epoll_event)));
	if (!alive_events)
	{
		std::cout << "calloc memory failed for epoll_events!" << std::endl;
		return;
	}
	// if loop_flag_ is false, will exit this loop
	while (loopFlag_)
	{
		// call epoll_wait and return ready socket
		int num = epoll_wait(efd_, alive_events, MaxEvents(), EpollWaitTime());

		for (int i = 0; i < num; ++i)
		{
			// get fd
			int fd = alive_events[i].data.fd;
			// get events(readable/writeable/error)
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
				std::cout << "epollin" << std::endl;
				if (fd == handle_)
				{
					// listen fd coming connections
					onSocketAccept();
				}
				else
				{
					// other fd read event coming, meaning data coming
					onSocketRead(fd);
				}
			}
			else if ( events & EPOLLOUT )
			{
				std::cout << "epollout" << std::endl;
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



