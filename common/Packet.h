/********************************************************************************
> FileName:	Packet.h
> Author:	Mingping Zhang
> Email:	mingpingzhang@163.com
> Create Time:	Mon Mar 13 09:46:40 2023
********************************************************************************/
#ifndef PACKET_H
#define PACKET_H

#include <memory>
#include <string>
#include <functional>

class Packet 
{
	public:
		Packet(){};
		Packet(const std::string& message)
			: message_(message)  {}
		Packet(int fd, const std::string& message)
			: fd_(fd),
			message_(message) {}
	public:
		int fd()
		{return fd_;}
		void setFD(const int value)
		{ fd_ = value; }
		const std::string& message()const
		{ return message_; }
		void setMessage(const std::string& value)
		{ message_ = value; }
	private:
		int fd_ { -1 };     // meaning socket
		std::string message_;   // real binary content
} ;

typedef std::shared_ptr<Packet> PacketPtr;

static const uint32_t kEpollWaitTime = 10; // epoll wait timeout 10 ms
static const uint32_t kMaxEvents = 100;    // epoll wait return max size

using callback_recv_t = std::function<void(const PacketPtr& data)>;
#endif//PACKET_H
