#pragma once

#ifdef USE_RING_BUF

class LogcatLogBuffer
{
public:
	LogcatLogBuffer();
	void StartWork();
	void StopWork();
	void GetLogBuffer(char** p, size_t* c);
	size_t GetLogData(char* p, size_t buf_size);
	void AddLogData(const char* szLog, size_t cbLog);
	~LogcatLogBuffer()
	{
		DeleteCriticalSection(&m_cs);
		delete [] buffer;
	}
private:
	void init();
	char*  buffer;
	char* const start_pos;
	char* const end_pos;
	char* data_pos;
	size_t cb, max_cb;
	CRITICAL_SECTION m_cs;
	HANDLE m_hNewDataEvent;
	HANDLE m_hFreeBufferEvent;
};

extern LogcatLogBuffer gLogcatLogBuffer;

#endif //USE_RING_BUF
