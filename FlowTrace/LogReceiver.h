#pragma once

class LogReceiver
{
public:
	LogReceiver() { m_working = false; InitializeCriticalSectionAndSpinCount(&cs, 0x00000400); }
	void start(bool reset);
	void stop();
	BOOL tryLock() { return TryEnterCriticalSection(&cs); }
	void lock() { EnterCriticalSection(&cs); }
	void unlock() {  LeaveCriticalSection(&cs); }
	bool working() { return m_working; }
	void purgePackets();
private:
	bool m_working;
	CRITICAL_SECTION cs;
};

extern LogReceiver gLogReceiver;
