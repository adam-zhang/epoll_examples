/********************************************************************************
> FileName:	EpollTcpBase.h
> Author:	Mingping Zhang
> Email:	mingpingzhang@163.com
> Create Time:	Mon Mar 13 09:49:16 2023
********************************************************************************/
#ifndef EPOLLTCPBASE_H
#define EPOLLTCPBASE_H

#include "Packet.h"
#include <functional>

using callback_recv_t = std::function<void(const PacketPtr& data)>;

class EpollTcpBase {
public:
    EpollTcpBase()                                     = default;
    EpollTcpBase(const EpollTcpBase& other)            = delete;
    EpollTcpBase& operator=(const EpollTcpBase& other) = delete;
    EpollTcpBase(EpollTcpBase&& other)                 = delete;
    EpollTcpBase& operator=(EpollTcpBase&& other)      = delete;
    virtual ~EpollTcpBase()                            = default; 

public:
    virtual bool start() = 0;
    virtual bool stop()  = 0;
    virtual int32_t sendData(const PacketPtr& data) = 0;
    virtual void registerOnRecvCallback(callback_recv_t callback) = 0;
    virtual void unregisterOnRecvCallback() = 0;
};


#endif//EPOLLTCPBASE_H
