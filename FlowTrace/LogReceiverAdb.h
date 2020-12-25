#pragma once

#include "..\Logcat\adb\client\adb_commandline.h"
#include "LogReceiver.h"
#include "WorkerThread.h"
#include "Archive.h"
#include "LogcatLogBuffer.h"

struct LOG_PARCER {
	LOG_PARCER(int c) {
		_buf_size = c;
		_buf = new char[_buf_size + 8];
		reset();
	}
	~LOG_PARCER() {
		delete[] _buf;
	}
	char* buf() { return _buf; }
	int size() { return _size; }
	int getLine(const char* sz, size_t cb, bool endWithNewLine) //return number of bytes paresed in sz
	{
		int i = 0;
		const char* end = sz + cb;
		bool endOfLine = false;
		//skip leading new lines
		if (_size == 0)
		{
			for (; i < cb && (sz[i] == '\n' || sz[i] == '\r'); i++)
			{
			}
		}
		for (; i < cb && (sz[i] != '\n' && sz[i] != '\r'); i++)
		{
			if (_size < _buf_size)
			{
				_buf[_size] = sz[i];
				_size++;
			}
			else
			{
				ATLASSERT(false);
				break;
			}

		}
		riched = (_size > 0 && i < cb && (sz[i] == '\n' || sz[i] == '\r'));
		//skip trailing new lines
		for (; i < cb && (sz[i] == '\n' || sz[i] == '\r'); i++)
		{
		}
		if (riched)
		{
			if (endWithNewLine)
			{
				_buf[_size] = '\n';
				_size++;
			}
		}
		_buf[_size] = 0;
		return i;
	}
	void reset() { _size = 0; _buf[0] = 0; riched = false; }
	bool compleated() { return riched; }
private:
	bool riched;
	char* _buf;
	int _buf_size;
	int _size;
};

class AdbStreamCallback : public StandardStreamsCallbackInterface {
public:
	bool OnStdout(char* buffer, int length) {
		return HundleStream(buffer, length);
	}

	bool OnStderr(char* buffer, int length) {
		return HundleStream(buffer, length);
	}

	int Done(int status) {
		StandardStreamsCallbackInterface::Done(status);
		return status;
	}

protected:
	virtual bool HundleStream(char* buffer, int length) = 0;
};

class PsStreamCallback : public AdbStreamCallback {
public:
	void GetStreamBuf(char** p, size_t* c) {
		*p = local_buffer;
		*c = sizeof(local_buffer) - 1;
	}

protected:
	char local_buffer[4 * 512 + 1]; //4*512
	bool HundleStream(char* buffer, int length);
};

class LogcatStreamCallback : public AdbStreamCallback {
public:
	void GetStreamBuf(char** p, size_t* c);
protected:
#ifndef USE_RING_BUF
	char local_buffer[4 * 512 + 1]; //4*512
#endif //USE_RING_BUF
	bool HundleStream(char* buffer, int length);
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
#ifdef USE_RING_BUF
	LogcatLogConsumer logcatLogConsumer;
#endif //USE_RING_BUF
	LogcatPsCommand logcatPsCommand;
};

extern LogReceiverAdb gLogReceiverAdb;