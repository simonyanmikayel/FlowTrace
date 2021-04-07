#include "stdafx.h"
#include "LogReceiverAdb.h"
#include "Settings.h"
#include "Helpers.h"

void LogcatLogSupplier::Terminate()
{
	streamCallback.Done(0);
}

void LogcatStreamCallback::GetStreamBuf(char** p, size_t* c)
{
#ifdef USE_RING_BUF
	gLogcatLogBuffer.GetLogBuffer(p, c);
#else
	* p = local_buffer;
	*c = sizeof(local_buffer) - 1;
#endif //USE_RING_BUF
}

bool LogcatStreamCallback::HundleStream(char* szLog, int cbLog)
{
#ifdef USE_RING_BUF
	gLogcatLogBuffer.AddLogData(szLog, cbLog);
#else
	gLogReceiverAdb.HandleLogData(szLog, cbLog);
#endif //USE_RING_BUF
	return gLogReceiverAdb.working();
}

//tcp.port=5037
/*
adb logcat -g
/dev/log/main: ring buffer is 2048Kb (2047Kb consumed), max entry is 5120b, max payload is 4076b
/dev/log/system: ring buffer is 256Kb (92Kb consumed), max entry is 5120b, max payload is 4076b
*/
//adb logcat -G 512K or adb logcat -G 4M
static const char* cmdLogcatClear = "logcat -c";
//static const char* cmdLogcatClear[]{ "logcat", "-b", "all", "-c" }; //adb logcat -b all -c
//static const char* cmdStartServer[]{ "start-server" };
//-v long: Display all metadata fields and separate messages with blank lines.
//adb logcat -v long FLOW_TRACE:* *:S
//adb logcat -v long FLOW_TRACE:S
//adb logcat -v long -f d:\temp\log.txt
//static const char* cmdLogcatLog[]{ "logcat", "-v", "long", "FLOW_TRACE:*", "*:S" }; //show only FLOW_TRACE tag
//static const char* cmdLogcatLog[]{ "logcat", "-v", "long", "FLOW_TRACE:S" }; //hide FLOW_TRACE tag
static const char* cmdLogcatLog = "logcat -v long"; // adb logcat -v long
static char cmd[1204];
void LogcatLogSupplier::Work(LPVOID pWorkParam)
{
	resetAtStart = *((bool*)pWorkParam);

	if (resetAtStart)
	{
		char* p = cmd;
		size_t c = _countof(cmd);
		Helpers::strCpy(p, gSettings.GetAdbArg(), c);
		Helpers::strCpy(p, " ", c);
		Helpers::strCpy(p, cmdLogcatClear, c);
		adb_commandline(cmd, &streamCallback);
	}
	while (IsWorking()) {
		char* p = cmd;
		size_t c = _countof(cmd);
		Helpers::strCpy(p, gSettings.GetAdbArg(), c);
		Helpers::strCpy(p, " ", c);
		if (gSettings.GetApplyLogcutFilter())
		{
			Helpers::strCpy(p, cmdLogcatLog, c);

			StringList& stringList = gSettings.getAdbFilterList();
			for (int i = 0; i < stringList.getItemCount(); i++) {
				Helpers::strCpy(p, " ", c); 
				Helpers::strCpy(p, stringList.getItem(i), c);
			}
		}
		else
		{
			Helpers::strCpy(p, cmdLogcatLog, c);
		}
		adb_commandline(cmd, &streamCallback);
		SleepThread(1000);
	}
}
