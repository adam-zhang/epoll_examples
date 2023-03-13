#ifndef APPDEF_H
#define APPDEF_H

constexpr uint32_t EpollWaitTime()
{
	return 10; // epoll wait timeout 10 ms
}

constexpr uint32_t MaxEvents()
{
	return 100;    // epoll wait return max size
}

#endif//APPDEF_H
