#pragma once

#include "LogReceiver.h"
#include "LogRecorder.h"
#include "WorkerThread.h"
#include "Archive.h"


class NetThread : public WorkerThread
{
public:
    virtual void Terminate();

protected:
    SOCKET s;
};

class UdpThread : public NetThread
{
	virtual void Work(LPVOID pWorkParam);
#ifdef  USE_RECORDER_THREAD
	LogRecorder recorder;
#else
	NET_PACK thePack;
#endif
public:
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
	void start();
	void stop();
protected:
	void add(NetThread* pNetThread);
	NetThread* pThreads[32];
	int cThreads = 0;
};
