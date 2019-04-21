#pragma once

#include "..\Logcat\adb\client\adb_commandline.h"
#include "LogReceiver.h"
#include "WorkerThread.h"
#include "Archive.h"


class AdbStreamCallback : public StandardStreamsCallbackInterface {
public:
	bool OnStdout(const char* buffer, int length) {
		return HundleStream(buffer, length);
	}

	bool OnStderr(const char* buffer, int length) {
		return HundleStream(buffer, length);
	}

	int Done(int status) {
		StandardStreamsCallbackInterface::Done(status);
		return status;
	}

	char local_buffer[512]; //4*512
	int GetBufSize() { return sizeof(local_buffer); };
	char* GetBuf() { return local_buffer; };

protected:
	virtual bool HundleStream(const char* buffer, int length) = 0;
};

class PsStreamCallback : public AdbStreamCallback {
protected:
	bool HundleStream(const char* buffer, int length);
};
class LogcatStreamCallback : public AdbStreamCallback {
protected:
	bool HundleStream(const char* buffer, int length);
};

class LogcatLogThread : public WorkerThread
{
public:
	virtual void Work(LPVOID pWorkParam);
	virtual void Terminate();
protected:
	LogcatStreamCallback streamCallback;
};

class PsThread : public WorkerThread
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
	void start(bool reset);
	void stop();
protected:
	LogcatLogThread logcatLogThread;
	PsThread psThread;
};

extern LogReceiverAdb gLogReceiverAdb;
