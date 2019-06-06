#pragma once

#include "WorkerThread.h"
#include "Archive.h"

#define NET_PACK_COUNT 512

//#define USE_RECORDER_THREAD

#ifdef  USE_RECORDER_THREAD
class LogRecorder : public WorkerThread
{
public:
	LogRecorder();
	void Terminate() override;
	void Work(LPVOID pWorkParam) override;
	NET_PACK* GetNetPak();
	BOOL tryLock() { return TryEnterCriticalSection(&cs); }
	void lock() { EnterCriticalSection(&cs); }
	void unlock() { LeaveCriticalSection(&cs); }
private:

	CRITICAL_SECTION cs;
	int firstPack, lastPack;
	NET_PACK packs[NET_PACK_COUNT];
	void ProcessLog();
	bool isOK;
};
#endif

#ifdef  USE_RECORDER_THREAD
void SetPackState(NET_PACK* pack, NET_PACK_STATE s); 
#else
inline void SetPackState(NET_PACK* pack, NET_PACK_STATE s) {}
#endif

bool RecordNetPack(NET_PACK* pack);