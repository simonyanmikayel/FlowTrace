#include "stdafx.h"
#include "LogRecorder.h"
#include "LogReceiver.h"
#include "Archive.h"

bool RecordNetPack(NET_PACK* pack)
{
	bool isOK = true;
	int cb_parced = 0;
	LOG_REC_NET_DATA* pLogData = (LOG_REC_NET_DATA*)(pack->data);

	while (cb_parced < pack->info.data_len)
	{
		if (pLogData->len > (pack->info.data_len - cb_parced))
		{
			isOK = false;
			break;
		}
		if (!gArchive.appendNet(pLogData, &pack->si_other, false, 0, &pack->info))
		{
			isOK = false;
			break;
		}
		cb_parced += pLogData->len;
		pLogData = (LOG_REC_NET_DATA*)(((char*)pLogData) + pLogData->len);
	}
	return isOK;
}

#ifdef  USE_RECORDER_THREAD
void SetPackState(NET_PACK* pack, NET_PACK_STATE s)
{
	gLogReceiver.lock();
	pack->state = s;
	gLogReceiver.unlock();
}

inline int NextIndex(int i)
{
	i++;
	if (i == NET_PACK_COUNT)
		i = 0;
	return i;
}

LogRecorder::LogRecorder() 
{ 
	lastPack = firstPack = 0;
	ZeroMemory(packs, sizeof(packs));
	isOK = true;
	InitializeCriticalSectionAndSpinCount(&cs, 0x00000400);
}

void LogRecorder::Terminate()
{

}

void LogRecorder::Work(LPVOID pWorkParam)
{
	//SetThreadPriority(THREAD_PRIORITY_HIGHEST);
	while (IsWorking())
	{
		SleepThread(INFINITE);
		if (!IsWorking())
			break;
		ProcessLog();
	}
}

void LogRecorder::ProcessLog()
{
	lock();
	if (packs[firstPack].state == NET_PACK_READY)
	{
		//stdlog("ProcessLog %d\n", firstPack);
		NET_PACK* pack = &packs[firstPack];
		isOK = RecordNetPack(pack);
		packs[firstPack].state = NET_PACK_FREE;
		firstPack = NextIndex(firstPack);
		if (packs[firstPack].state == NET_PACK_READY)
			ResumeThread();
	}
	unlock();
}

NET_PACK* LogRecorder::GetNetPak()
{
	if (!isOK)
		return nullptr;

	gLogReceiver.lock();

	if (packs[lastPack].state != NET_PACK_FREE)
	{
		int nextIndaex = NextIndex(lastPack);
		ATLASSERT(packs[lastPack].state == NET_PACK_READY);
		if (packs[nextIndaex].state == NET_PACK_FREE)
		{
			lastPack = nextIndaex;
		}
		else
		{
			ATLASSERT(lastPack == firstPack);
			ProcessLog();
		}
	}
	ATLASSERT(packs[lastPack].state == NET_PACK_FREE);
	packs[lastPack].state = NET_PACK_BUZY;

	gLogReceiver.unlock();

	return &packs[lastPack];
}
#endif

