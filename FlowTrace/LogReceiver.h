#pragma once

class LogReceiver
{
public:
	LogReceiver() { m_working = false; InitializeCriticalSectionAndSpinCount(&cs, 0x00000400); }
	void start(bool reset);
	void stop();
	void lock() { EnterCriticalSection(&cs); }
	void unlock() { LeaveCriticalSection(&cs); }
	bool working() { return m_working; }
protected:
	bool m_working;
	CRITICAL_SECTION cs;
};

extern LogReceiver gLogReceiver;
