#pragma once

#include "LogReceiver.h"
#include "WorkerThread.h"
#include "Archive.h"

//#define USE_TCP

#ifdef USE_TCP
#define MAX_NET_BUF 16*1024  //maximum TCP window size in Microsoft Windows 2000 is 17,520 bytes 
#else
#define MAX_NET_BUF 16*1024  //max UDP datagam is 65515 Bytes
#endif
#define NET_BUF_COUNT 1

class NetThread : public WorkerThread
{
public:
    virtual void Terminate();
	virtual void purgePackets() = 0;

protected:
    SOCKET s;
};

class UdpThread : public NetThread
{
	virtual void Work(LPVOID pWorkParam);
    char netBuf[NET_BUF_COUNT][MAX_NET_BUF + sizeof(NET_PACK_INFO)];
	int curBuf;
	bool havePacket;
	bool isOK;
	bool appendBuf(char* buf);
	struct sockaddr_in si_other;
public:
	void purgePackets();
	UdpThread();
};

#ifdef USE_TCP

class TcpListenThread : public NetThread
{
    virtual void Work(LPVOID pWorkParam);
public:
    TcpListenThread();
};

class TcpReceiveThread : public NetThread
{
    virtual void Work(LPVOID pWorkParam);
    char buf[MAX_NET_BUF + sizeof(NET_PACK_INFO)];
public:
    TcpReceiveThread(SOCKET sclientSocket);
};

#endif //USE_TCP

class LogReceiverNet
{
public:
	void start(bool reset);
	void stop();
	void purgePackets();
protected:
	void add(NetThread* pNetThread);
};

