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

#ifdef USE_RAW_TCP

class RawTcpListenThread : public NetThread
{
    virtual void Work(LPVOID pWorkParam);
public:
    RawTcpListenThread();
};

class RawTcpReceiveThread : public NetThread
{
    virtual void Work(LPVOID pWorkParam);
public:
    char buffer[4096];
    LOG_REC_SERIAL_DATA logData;
    RawTcpReceiveThread(SOCKET sclientSocket);
private:
    int dummyPid;
};

#endif //USE_RAW_TCP

class LogReceiverNet
{
public:
	void start();
	void stop();
    void add(NetThread* pNetThread);
    int threadCount() { return cThreads; }
protected:
	NetThread* pThreads[32];
	int cThreads = 0;
};
