#pragma once

#include "..\Logcat\adb\client\adb_commandline.h"
#include "LogReceiver.h"
#include "WorkerThread.h"
#include "Archive.h"
#include "LogParcer.h"

struct MetaDataInfo {
	int pid, tid, size;
	long long totMsec;
	boolean operator== (const MetaDataInfo& other) {
		return totMsec == other.totMsec && pid == other.pid && tid == other.tid && size == other.size;
	}
	void reset() { totMsec = 0; }
};


class AdbStreamCallback : public StandardStreamsCallbackInterface {
public:
	bool OnStdout(char* buffer, int length) {
		return HundleStream(buffer, length, false);
	}

	bool OnStderr(char* buffer, int length) {
		return HundleStream(buffer, length, true);
	}

	int Done(int status) {
		StandardStreamsCallbackInterface::Done(status);
		return status;
	}

protected:
	virtual bool HundleStream(char* buffer, int length, bool isError) = 0;
};

class PsStreamCallback : public AdbStreamCallback {
protected:
	bool HundleStream(char* buffer, int length, bool isError);
};

class LogcatStreamCallback : public AdbStreamCallback {
protected:
	bool HundleStream(char* buffer, int length, bool isError);
};

class LogcatLogSupplier : public WorkerThread
{
public:
	virtual void Work(LPVOID pWorkParam);
	virtual void Terminate();
protected:
	bool resetAtStart;
	LogcatStreamCallback streamCallback;
};

class LogcatLogConsumer : public WorkerThread
{
public:
	virtual void Work(LPVOID pWorkParam);
	virtual void Terminate();
private:
	char local_buffer[4 * 512]; //4*512
};

class LogcatPsCommand : public WorkerThread
{
public:
	virtual void Work(LPVOID pWorkParam);
	virtual void Terminate();
protected:
	PsStreamCallback streamCallback;
};

class LogReceiverAdb
{
public:
	LogReceiverAdb() { m_working = false; }
	void start(bool reset);
	void stop();
	void HandleLogData(const char* szLog, size_t cbLog);
	bool working() {
		return m_working;
	}
protected:
	bool m_working;
	LogcatLogSupplier logcatLogSupplier;
	LogcatPsCommand logcatPsCommand;
};

extern LogReceiverAdb gLogReceiverAdb;