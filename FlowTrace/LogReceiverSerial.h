#pragma once

#include "LogReceiver.h"
#include "LogRecorder.h"
#include "WorkerThread.h"
#include "Archive.h"
#include "LogParcer.h"

class SerialThread : public WorkerThread
{
public:
	SerialThread(ComPort& comPort);
	virtual void Terminate();
	virtual void Work(LPVOID pWorkParam); 

	void TraceLog(const char* szLog, int cbLog);
	void serial_close();
	char* serial_open();
	char* serial_configure();
	ComPort& m_comPort;
	HANDLE serport;
	DWORD flags;
	DWORD len;			       /* how much data that was */
	char buffer[4096];		       /* the data read from the handle */
	int readerr;		       /* lets us know about read errors */
	LogParcer logParcer;
	LOG_REC_SERIAL_DATA logData;
	int nn;
};

class LogReceiverSerial
{
public:
	void start();
	void stop();
protected:
	void add(SerialThread* pThread);
	SerialThread* pThreads[32];
	int cThreads = 0;
};

