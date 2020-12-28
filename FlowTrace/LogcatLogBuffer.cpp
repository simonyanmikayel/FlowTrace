#include "stdafx.h"
#include "LogReceiverAdb.h"

#ifdef USE_RING_BUF

LogcatLogBuffer gLogcatLogBuffer;

LogcatLogBuffer::LogcatLogBuffer()
{
	BOOL bManualReset = TRUE;
	InitializeCriticalSectionAndSpinCount(&m_cs, 0x00000400);
	m_hNewDataEvent = CreateEvent(NULL, bManualReset, FALSE, NULL);
	m_hFreeBufferEvent = CreateEvent(NULL, bManualReset, FALSE, NULL);
	const int buffer_size = 1024 * 1024 * 128;
	buffer = new char[buffer_size];
	start_pos = buffer;
	end_pos = buffer + buffer_size;
}

void LogcatLogBuffer::init()
{
	EnterCriticalSection(&m_cs);
	data_pos = start_pos;
	max_cb = cb = 0;
	LeaveCriticalSection(&m_cs);
}

void LogcatLogBuffer::StartWork(){
	
	init();
	ResetEvent(m_hNewDataEvent);
	ResetEvent(m_hFreeBufferEvent);
}

void LogcatLogBuffer::StopWork()
{
	init();
	SetEvent(m_hNewDataEvent);
	SetEvent(m_hFreeBufferEvent);
}

void LogcatLogBuffer::GetLogBuffer(char** p, size_t* cnt)
{
	size_t &c = *cnt;
	c = 0;
	while (c == 0 && gLogReceiverAdb.working())
	{
		EnterCriticalSection(&m_cs);
		if (cb > max_cb)
		{
			stdlog("GetLogBuffer cb = %d\n", cb);
			max_cb = cb;
		}
		char* data_end = data_pos + cb;
		if (data_end < end_pos)
		{
			c = end_pos - data_end;
			*p = data_end;
		}
		else
		{
			stdlog("->GetLogBuffer buffer over = %d\n", data_end - end_pos);
			data_end = start_pos + (data_end - end_pos);
			ATLASSERT(data_end <= data_pos);
			if (data_end < data_pos)
			{
				c = data_pos - data_end;
				*p = data_end;
			}
		}
		LeaveCriticalSection(&m_cs);
		if (c == 0 && gLogReceiverAdb.working())
		{
			stdlog("->GetLogBuffer wait for m_hFreeBufferEvent cb = %d\n", cb);
			WaitForSingleObject(m_hFreeBufferEvent, INFINITE);
			ResetEvent(m_hFreeBufferEvent);
			//stdlog("<-GetLogBuffer wait for m_hFreeBufferEvent cb = %d\n", cb);
		}
	}
	
}

void LogcatLogBuffer::AddLogData(const char* szLog, size_t cbLog)
{
	bool haveBuf = false;
	EnterCriticalSection(&m_cs);
	char* data_end = data_pos + cb;
	if (data_end < end_pos)
	{
		haveBuf = (cbLog <= size_t(end_pos - data_end));
	}
	else
	{
		data_end = start_pos + (data_end - end_pos);
		ATLASSERT(data_end < data_pos);
		if (data_end < data_pos)
			haveBuf = (cbLog <= size_t(data_pos - data_end));
	}
	ATLASSERT(szLog = data_end);
	ATLASSERT(haveBuf);
	if (haveBuf)
	{
		cb += cbLog;
	}
	//stdlog("AddLogData->pos=%d cb=%d cbLog=%d %.*s\n", int(data_pos - start_pos), cb, cbLog, cbLog, szLog);
	LeaveCriticalSection(&m_cs);
	//stdlog("AddLogData->pos=%d cb=%d cbLog=%d\n", int(data_pos - start_pos), cb, cbLog);
	SetEvent(m_hNewDataEvent);
}

size_t LogcatLogBuffer::GetLogData(char* p, size_t buf_size)
{
	size_t c = 0;
	while (c == 0 && buf_size && gLogReceiverAdb.working())
	{
		EnterCriticalSection(&m_cs);
		if (cb)
		{
			char* data_end = data_pos + cb;
			size_t c1 = (data_end > end_pos) ? end_pos - data_pos : data_end - data_pos;
			c1 = std::min(c1, buf_size);
			memcpy(p, data_pos, c1);
			data_pos += c1;
			buf_size -= c1;
			cb -= c1;
			c += c1;
			if (data_pos == end_pos)
				data_pos = start_pos;
			if (cb && buf_size && (data_end > end_pos))
			{
				size_t c2 =  (data_end - end_pos); //extra data at the  beggining of ciclyck buffer
				c2 = std::min(c2, buf_size);
				ATLASSERT(data_pos = start_pos);
				memcpy(p + c1, start_pos, c2);
				data_pos = start_pos;
				data_pos += c2;
				buf_size -= c2;
				cb -= c2;
				c += c2;
			}
		}
		//if (c) stdlog("GetLogData->pos=%d cb=%d c=%d %.*s\n", int(data_pos - start_pos), cb, c, c, p);
		LeaveCriticalSection(&m_cs);
		if (c == 0 && gLogReceiverAdb.working())
		{
			//stdlog("->GetLogData wait for m_hNewDataEvent cb = %d\n", cb);
			WaitForSingleObject(m_hNewDataEvent, INFINITE);
			ResetEvent(m_hNewDataEvent);
			//stdlog("<-GetLogData wait for m_hNewDataEvent cb = %d\n", cb);
		}
	}
	//stdlog("GetLogData->pos=%d cb=%d c=%d\n", int(data_pos - start_pos), cb, c);
	SetEvent(m_hFreeBufferEvent);
	return c;
}

#endif //USE_RING_BUF
